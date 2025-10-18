#include <stdafx.h>

#include <common/sync/spmc_queue.h>

using namespace tjs::common;

namespace {
    struct SmallPod {
        int x{};
        int y{};
        SmallPod() = default;
        SmallPod(int a, int b) : x(a), y(b) {}
    };

    // Simple payload carrying a sequence number to validate ordering
    struct SeqPayload {
        std::uint64_t seq{};
        explicit SeqPayload(std::uint64_t s = 0)
            : seq(s) {
        }
    };

    enum class MsgKind : char { A = 1, B = 2, C = 3, D = 4 };
    // Choose a large capacity to avoid overrun in ordering tests
    static constexpr std::size_t kCapacityLarge = 16384;

    template <std::size_t MsgBufSize>
    using QueueLarge = sync::spmc_queue<MsgKind, kCapacityLarge, MsgBufSize>;
}

TEST(SpmcQueueTests, SingleThreadedOrderAndDrain) {
    using Q = QueueLarge<64>;
    Q q;

    // Start reading from 0
    Q::reader r = q.connect();

    const std::size_t N = 5000;
    for (std::size_t i = 0; i < N; ++i) {
        q.push<SeqPayload>(MsgKind::A, i);
    }

    std::vector<std::uint64_t> seen;
    seen.reserve(N);

    r.read_all([&](const Q::message_t& m){
        const auto& p = m.get<SeqPayload>();
        seen.push_back(p.seq);
    });

    ASSERT_EQ(seen.size(), N);
    for (std::size_t i = 0; i < N; ++i) {
        EXPECT_EQ(seen[i], i);
    }
}


TEST(SpmcQueueTests, Multithreaded_SingleProducer_ManyConsumers_AllSeeAll) {
    using Q = QueueLarge<64>;
    Q q;

    constexpr std::size_t N = 20000;
    constexpr int Readers = 4;

    // Create readers starting at 0 to read everything
    std::vector<Q::reader> readers;
    readers.reserve(Readers);
    for (int i = 0; i < Readers; ++i) {
        readers.emplace_back(q.connect());
    }

    std::barrier sync_point(Readers + 1);

    // Reader threads: each should see all N messages, in order
    std::vector<std::thread> rthreads;
    std::vector<std::size_t> counts(Readers, 0);
    std::vector<std::uint64_t> last_seq(Readers, 0);

    for (int i = 0; i < Readers; ++i) {
        rthreads.emplace_back([&, i]{
            sync_point.arrive_and_wait();

            std::size_t local = 0;
            std::optional<std::uint64_t> prev;
            while (local < N) {
                if (readers[i].read([&](const Q::message_t& m){
                        const auto& p = m.get<SeqPayload>();
                        if (prev) {
                            // strictly increasing
                            ASSERT_GT(p.seq, *prev);
                        }
                        prev = p.seq;
                        ++local;
                    })) {
                    // consumed one
                } else {
                    // no item; yield briefly
                    std::this_thread::yield();
                }
            }
            counts[i] = local;
            last_seq[i] = *prev;
        });
    }

    // Producer
    std::thread producer([&]{
        sync_point.arrive_and_wait();
        for (std::size_t i = 0; i < N; ++i) {
            q.push<SeqPayload>(MsgKind::A, i);
        }
    });

    producer.join();
    for (auto& t : rthreads) {
        t.join();
    }

    for (int i = 0; i < Readers; ++i) {
        EXPECT_EQ(counts[i], N);
        EXPECT_EQ(last_seq[i], N - 1);
    }
}

TEST(SpmcQueue, OverrunSkipsOverwrittenSlotAndStaysConsistent) {
    // Small capacity to force overrun
    using Q = sync::spmc_queue<MsgKind, 8, 64>;
    Q q;

    // Reader from 0 (far behind soon)
    typename Q::reader r{&q, 0};

    const std::size_t total = 32;
    for (std::size_t i = 0; i < total; ++i) {
        q.template push<SeqPayload>(MsgKind::A, i);
    }

    std::vector<std::uint64_t> seen;
    r.read_all([&](const typename Q::message_t& m){
        const auto& p = m.template get<SeqPayload>();
        seen.push_back(p.seq);
    });

    // With the >= overrun fix that skips the overwritten slot,
    // the earliest readable seq is (tail - capacity + 1).
    // tail == total
    const std::uint64_t tail = q.current_idx();
    ASSERT_EQ(tail, total);

    const std::uint64_t expected_first = tail - 8 + 1; // skip the slot being overwritten
    ASSERT_FALSE(seen.empty());
    EXPECT_EQ(seen.front(), expected_first);

    // The last must be tail-1 (latest published)
    EXPECT_EQ(seen.back(), tail - 1);

    // And the sequence must be strictly increasing
    for (std::size_t i = 1; i < seen.size(); ++i) {
        ASSERT_GT(seen[i], seen[i - 1]);
    }
}

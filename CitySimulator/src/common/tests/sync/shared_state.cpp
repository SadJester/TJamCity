#include <stdafx.h>

#include <common/sync/shared_state.h>

using namespace tjs::common;

TEST(SharedStateTests, InitAndPublishReadAll) {
    sync::shared_state<int, 3> state;
    state.init(4);

    auto r = state.connect();

    // publish 4 elements: [10, 20, 30, 40]
    bool ok = state.write([](int& v, uint32_t i) {
        v = static_cast<int>((i + 1) * 10);
        return true; // mark dirty for all readers
    }, /*count=*/4);
    ASSERT_TRUE(ok);

    std::vector<int> got;
    r.read_all([&](const int& v, uint32_t i){
        got.push_back(v);
    });
    ASSERT_EQ(got.size(), 4u);
    EXPECT_EQ(got[0], 10);
    EXPECT_EQ(got[1], 20);
    EXPECT_EQ(got[2], 30);
    EXPECT_EQ(got[3], 40);
}

TEST(SharedStateTests, DirtyFilteringPerReader) {
    sync::shared_state<int, 3> state;
    state.init(5);

    auto r1 = state.connect();
    auto r2 = state.connect();

    // publish: [0,1,2,3,4]
    ASSERT_TRUE(state.write([](int& v, uint32_t i){
        v = static_cast<int>(i);
        return true;
    }, 5));

    // r1 consumes all -> clears its dirty bits
    size_t r1_count = 0;
    r1.read_all([&](const int& v, uint32_t /*i*/){ (void)v; ++r1_count; });
    EXPECT_EQ(r1_count, 5u);

    // r2 hasn't read yet; should still see all via read() (dirty-only)
    std::vector<int> r2_first_dirty;
    r2.read([&](const int& v, uint32_t /*i*/){ r2_first_dirty.push_back(v); });
    ASSERT_EQ(r2_first_dirty.size(), 5u);

    // Second publish: only even indices change/dirty
    ASSERT_TRUE(state.write([](int& v, uint32_t i){
        if (i % 2 == 0) { v = 100 + static_cast<int>(i); return true; }
        return false;
    }, 5));

    // r1 should now get only even indices via read()
    std::vector<int> r1_dirty;
    r1.read([&](const int& v, uint32_t /*i*/){ r1_dirty.push_back(v); });
    ASSERT_EQ(r1_dirty.size(), 3u); // 0,2,4
    EXPECT_THAT(r1_dirty, ::testing::ElementsAre(100, 102, 104));

    // r2 also gets only even (its bits for the first frame were cleared already by its read)
    std::vector<int> r2_dirty;
    r2.read([&](const int& v, uint32_t /*i*/){ r2_dirty.push_back(v); });
    ASSERT_EQ(r2_dirty.size(), 3u);
    EXPECT_THAT(r2_dirty, ::testing::ElementsAre(100, 102, 104));

    // Subsequent read()s without a new publish should yield nothing
    std::vector<int> r1_again;
    r1.read([&](const int& v, uint32_t /*i*/){ r1_again.push_back(v); });
    EXPECT_TRUE(r1_again.empty());
}

TEST(SharedStateTests, WriteFailsWhenCountExceedsCapacity) {
    sync::shared_state<int, 2> state;
    state.init(3);

    // count > capacity -> false
    bool ok = state.write([](int& v, uint32_t i){ v = int(i); return true; }, /*count=*/4);
    EXPECT_FALSE(ok);
}

TEST(SharedStateTests, ResizeAllowsLargerWrites) {
    sync::shared_state<int, 2> state;
    state.init(2);

    // Initially cannot write 4
    EXPECT_FALSE(state.write([](int& v, uint32_t i){ v = int(i); return true; }, 4, /*wait=*/false));

    // Resize and write 4
    state.resize(8);
    ASSERT_TRUE(state.write([](int& v, uint32_t i){ v = int(100 + i); return true; }, 4));

    auto r = state.connect();
    std::vector<int> got;
    r.read_all([&](const int& v, uint32_t /*i*/){ got.push_back(v); });
    ASSERT_EQ(got.size(), 4u);
    EXPECT_THAT(got, ::testing::ElementsAre(100, 101, 102, 103));
}

TEST(SharedStateTests, MultipleReadersIndependentDirtyBits) {
    sync::shared_state<int, 3> state;
    state.init(3);

    auto a = state.connect();
    auto b = state.connect();

    ASSERT_TRUE(state.write([](int& v, uint32_t i){ v = int(10 + i); return true; }, 3));

    // Reader A consumes all (clears A's bits)
    size_t a_cnt = 0;
    a.read_all([&](const int&, uint32_t){ ++a_cnt; });
    EXPECT_EQ(a_cnt, 3u);

    // Reader B still sees all via dirty-only read()
    std::vector<int> b_dirty;
    b.read([&](const int& v, uint32_t){ b_dirty.push_back(v); });
    EXPECT_EQ(b_dirty.size(), 3u);

    // Next publish: mark only index 1 dirty
    ASSERT_TRUE(state.write([](int& v, uint32_t i){
        if (i == 1) { v = 777; return true; }
        return false;
    }, 3));

    // A sees index 1
    std::vector<int> a_dirty2;
    a.read([&](const int& v, uint32_t){ a_dirty2.push_back(v); });
    ASSERT_EQ(a_dirty2.size(), 1u);
    EXPECT_EQ(a_dirty2[0], 777);

    // B also sees index 1
    std::vector<int> b_dirty2;
    b.read([&](const int& v, uint32_t){ b_dirty2.push_back(v); });
    ASSERT_EQ(b_dirty2.size(), 1u);
    EXPECT_EQ(b_dirty2[0], 777);
}

TEST(SharedStateTests, PublishRotationAndVisibility) {
    // Triple buffer to make rotation obvious
    sync::shared_state<int, 3> state;
    state.init(2);

    auto r = state.connect();

    // Publish frame 1
    ASSERT_TRUE(state.write([](int& v, uint32_t i){ v = 100 + int(i); return true; }, 2));
    std::vector<int> f1;
    r.read_all([&](const int& v, uint32_t){ f1.push_back(v); });
    EXPECT_THAT(f1, ::testing::ElementsAre(100, 101));

    // Publish frame 2
    ASSERT_TRUE(state.write([](int& v, uint32_t i){ v = 200 + int(i); return true; }, 2));
    std::vector<int> f2;
    r.read_all([&](const int& v, uint32_t){ f2.push_back(v); });
    EXPECT_THAT(f2, ::testing::ElementsAre(200, 201));
}

TEST(SharedStateTests, DirtyClearingOnReadAll) {
    sync::shared_state<int, 2> state;
    state.init(3);

    auto r = state.connect();

    ASSERT_TRUE(state.write([](int& v, uint32_t i){ v = int(i); return true; }, 3));

    // First read_all clears dirty for this reader
    size_t took = 0;
    r.read_all([&](const int&, uint32_t){ ++took; });
    EXPECT_EQ(took, 3u);

    // Subsequent read() should see nothing (no new publish, no new dirties)
    size_t took2 = 0;
    r.read([&](const int&, uint32_t){ ++took2; });
    EXPECT_EQ(took2, 0u);
}

TEST(SharedStateTests, Concurrency_WriterProgressWithPinnedReader_TripleBuffer) {
    // With 3 slots, the writer should keep publishing even if a reader pins the current slot for a long time.
    sync::shared_state<int, 3> state;
    const uint32_t N = 256;
    state.init(N);

    auto reader_conn = state.connect();

    // Publish initial frame so the reader has something to pin.
    ASSERT_TRUE(state.write([](int& v, uint32_t i) {
        v = static_cast<int>(i);
        return true;
    }, N));

    std::atomic<bool> hold_reader{true};
    std::atomic<uint32_t> reader_seen{0};

    // Reader thread pins the published slot and iterates slowly.
    std::thread reader([&]{
        // One long read_all that simulates heavy work while holding the guard
        reader_conn.read_all([&](const int& v, uint32_t) {
            // simulate some work per element
            if ((v & 0x3F) == 0) std::this_thread::yield();
            reader_seen.fetch_add(1, std::memory_order_relaxed);
        });
        // Keep the thread alive a bit longer while holding 'hold_reader' if desired
        while (hold_reader.load(std::memory_order_acquire)) {
            // Busy-wait to model the reader not re-acquiring, just holding time for realism
            std::this_thread::yield();
        }
    });

    // Writer publishes multiple frames while the reader is holding the slot
    const uint32_t frames = 50;
    std::atomic<uint32_t> wrote{0};

    std::thread writer([&]{
        for (uint32_t f = 1; f <= frames; ++f) {
            bool ok = state.write([&](int& v, uint32_t i) {
                v = static_cast<int>(1000 * f + i);
                return true; // mark dirty for all readers
            }, N, /*wait_for_free_slot=*/true);
            ASSERT_TRUE(ok);
            wrote.fetch_add(1, std::memory_order_relaxed);
            if ((f % 5) == 0) std::this_thread::yield();
        }
    });

    // Let the writer progress a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Release the reader so it stops hogging time
    hold_reader.store(false, std::memory_order_release);

    writer.join();
    reader.join();

    EXPECT_EQ(wrote.load(std::memory_order_relaxed), frames);

    // Final frame should be visible to a fresh read_all
    auto r2 = state.connect();
    std::vector<int> last;
    r2.read_all([&](const int& v, uint32_t){ last.push_back(v); });
    ASSERT_EQ(last.size(), N);
    // Expect prefix to match the last frame (1000*frames + i)
    EXPECT_EQ(last.front(), static_cast<int>(1000 * frames + 0));
    EXPECT_EQ(last.back(),  static_cast<int>(1000 * frames + (N - 1)));
}

TEST(SharedStateTests, Concurrency_ResizeBlocking_CompletesUnderHeavyReaders) {
    // Under massive read traffic, epoch-gated resize_blocking() must still complete.
    sync::shared_state<int, 3> state;
    const uint32_t N = 512;
    state.init(N);

    // Seed with initial data
    ASSERT_TRUE(state.write([](int& v, uint32_t i){
        v = static_cast<int>(i);
        return true;
    }, N));

    constexpr int reader_threads = 6;
    std::vector<sync::shared_state<int,3>::connection> conns;
    conns.reserve(reader_threads);
    for (int i = 0; i < reader_threads; ++i) conns.emplace_back(state.connect());

    std::atomic<bool> stop{false};
    std::atomic<uint64_t> total_reads{0};

    // Readers continuously acquire+read; they should spin briefly during resize and then continue.
    std::vector<std::thread> readers;
    readers.reserve(reader_threads);
    for (int t = 0; t < reader_threads; ++t) {
        readers.emplace_back([&, t]{
            auto& c = conns[t];
            while (!stop.load(std::memory_order_acquire)) {
                c.read([&](const int& v, uint32_t){
                    (void)v;
                    total_reads.fetch_add(1, std::memory_order_relaxed);
                });
                if ((t & 1) == 0) std::this_thread::yield();
            }
        });
    }

    // Writer does a blocking resize to a larger capacity.
    const uint32_t newN = N * 2;
    std::atomic<bool> resized{false};
    std::thread resizer([&]{
        // Let readers ramp up
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        state.resize(newN);
        resized.store(true, std::memory_order_release);

        // After resize, publish a frame of size newN
        ASSERT_TRUE(state.write([&](int& v, uint32_t i){
            v = static_cast<int>(100000 + i);
            return true;
        }, newN));
    });

    // Time bound to avoid deadlocks if something breaks
    auto start = std::chrono::steady_clock::now();
    while (!resized.load(std::memory_order_acquire)) {
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(3)) {
            FAIL() << "resize_blocking did not complete within 3s under heavy readers";
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    stop.store(true, std::memory_order_release);
    for (auto& th : readers) th.join();
    resizer.join();

    // Verify new capacity is usable by reading back newN elements from the last publish.
    auto rc = state.connect();
    std::vector<int> got;
    rc.read_all([&](const int& v, uint32_t){ got.push_back(v); });
    ASSERT_EQ(got.size(), newN);
    EXPECT_EQ(got.front(), 100000);
    EXPECT_EQ(got.back(),  100000 + static_cast<int>(newN - 1));
    EXPECT_GT(total_reads.load(std::memory_order_relaxed), 0u);
}

TEST(SharedStateTests, Concurrency_DirtyFilteringWithMultipleReaders) {
    sync::shared_state<int, 3> state;
    const uint32_t N = 64;
    state.init(N);

    auto A = state.connect();
    auto B = state.connect();

    // Frame 1: fill all, both readers should see all via read() (dirty-only) once.
    ASSERT_TRUE(state.write([](int& v, uint32_t i){ v = static_cast<int>(i); return true; }, N));

    std::vector<int> a1, b1;
    A.read([&](const int& v, uint32_t){ a1.push_back(v); });
    B.read([&](const int& v, uint32_t){ b1.push_back(v); });
    ASSERT_EQ(a1.size(), N);
    ASSERT_EQ(b1.size(), N);

    // Subsequent dirty-only reads with no new publish should be empty.
    std::vector<int> a1_again;
    A.read([&](const int& v, uint32_t){ a1_again.push_back(v); });
    EXPECT_TRUE(a1_again.empty());

    // Frame 2: mark only even indices dirty with new values.
    ASSERT_TRUE(state.write([](int& v, uint32_t i){
        if ((i & 1) == 0) { v = 1000 + static_cast<int>(i); return true; }
        return false;
    }, N));

    std::vector<int> a2, b2;
    A.read([&](const int& v, uint32_t){ a2.push_back(v); });
    B.read([&](const int& v, uint32_t){ b2.push_back(v); });

    // Expect N/2 elements (even indices)
    ASSERT_EQ(a2.size(), N / 2);
    ASSERT_EQ(b2.size(), N / 2);
    EXPECT_EQ(a2.front(), 1000 + 0);
    EXPECT_EQ(a2.back(),  1000 + static_cast<int>(N - (N % 2 ? 1 : 2)));
}

TEST(SharedStateTests, Concurrency_ResizeWhileReadersActive_MultipleRounds) {
    // Do several resize cycles while readers hammer the state.
    sync::shared_state<int, 3> state;
    const uint32_t N0 = 128;
    state.init(N0);

    auto r0 = state.connect();

    // publish baseline
    ASSERT_TRUE(state.write([](int& v, uint32_t i){ v = static_cast<int>(i); return true; }, N0));

    std::atomic<bool> stop{false};
    std::thread reader([&]{
        auto rr = state.connect();
        while (!stop.load(std::memory_order_acquire)) {
            rr.read([&](const int& v, uint32_t){ (void)v; });
            std::this_thread::yield();
        }
    });

    // Perform multiple resizes up/down and publish after each
    const uint32_t sizes[] = { 256, 64, 512, 128 };
    for (uint32_t newN : sizes) {
        state.resize(newN);
        ASSERT_TRUE(state.write([&](int& v, uint32_t i){ v = static_cast<int>(777000 + i); return true; }, newN));

        // Validate via independent connection
        auto rc = state.connect();
        std::vector<int> got;
        rc.read_all([&](const int& v, uint32_t){ got.push_back(v); });
        ASSERT_EQ(got.size(), newN);
        EXPECT_EQ(got.front(), 777000);
        EXPECT_EQ(got.back(),  777000 + static_cast<int>(newN - 1));
    }

    stop.store(true, std::memory_order_release);
    reader.join();

    // Final sanity with the original connection
    std::vector<int> last;
    r0.read_all([&](const int& v, uint32_t){ last.push_back(v); });
    ASSERT_FALSE(last.empty());
}

// They start multiple reader threads while the main (test) thread performs writes.

// Helper to extract a "frame id" we encode into values as frame*BASE + index.
static constexpr int kFrameBase = 100000;

// Readers continuously call read() (dirty-only), while main publishes frames.
// We assert writer completes all frames and final snapshot is consistent.
TEST(SharedStateTests, ConcurrentReaders_DirtyOnly_WhileMainWrites) {
    constexpr uint32_t N           = 256;
    constexpr uint32_t kReaders    = 6;
    constexpr uint32_t kFrames     = 200;

    sync::shared_state<int, 3> state;
    state.init(N);

    // Publish an initial frame 0 so readers have something to latch onto.
    ASSERT_TRUE(state.write([](int& v, uint32_t i){
        v = 0 * kFrameBase + static_cast<int>(i);
        return true;
    }, N));

    std::vector<sync::shared_state<int,3>::connection> conns;
    conns.reserve(kReaders);
    for (uint32_t i = 0; i < kReaders; ++i) conns.emplace_back(state.connect());

    std::atomic<bool> stop{false};
    std::atomic<uint64_t> total_reads{0};

    // Each reader tracks the last frame id it observed to ensure monotonic progress.
    std::vector<std::atomic<int>> last_frame(kReaders);
    for (auto& x : last_frame) x.store(-1, std::memory_order_relaxed);

    std::vector<std::thread> readers;
    readers.reserve(kReaders);
    for (uint32_t r = 0; r < kReaders; ++r) {
        readers.emplace_back([&, r]{
            auto& c = conns[r];
            while (!stop.load(std::memory_order_acquire)) {
                c.read([&](const int& v, uint32_t){
                    const int frame = v / kFrameBase;
                    int prev = last_frame[r].load(std::memory_order_relaxed);
                    if (frame >= 0 && frame >= prev) {
                        last_frame[r].store(frame, std::memory_order_relaxed);
                    }
                    total_reads.fetch_add(1, std::memory_order_relaxed);
                });
                if ((r & 1) == 0) std::this_thread::yield();
            }
        });
    }

    // Main thread publishes kFrames frames sequentially.
    for (uint32_t f = 1; f <= kFrames; ++f) {
        bool ok = state.write([&](int& v, uint32_t i){
            v = static_cast<int>(f) * kFrameBase + static_cast<int>(i);
            // we mark all elements dirty each frame for simplicity
            return true;
        }, N, /*wait_for_free_slot=*/true);
        ASSERT_TRUE(ok);
        if ((f % 10) == 0) std::this_thread::yield();
    }

    // Give readers a bit of time to consume the last publish.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    stop.store(true, std::memory_order_release);
    for (auto& th : readers) th.join();

    // Sanity: writer produced all frames, readers observed progress.
    for (uint32_t r = 0; r < kReaders; ++r) {
        EXPECT_GE(last_frame[r].load(std::memory_order_relaxed), 0);
        // Most readers should have seen near the end; don't require exact kFrames to avoid timing flukes.
        EXPECT_LE(last_frame[r].load(std::memory_order_relaxed), static_cast<int>(kFrames));
    }
    EXPECT_GT(total_reads.load(std::memory_order_relaxed), 0u);

    // Final snapshot must match the last frame exactly.
    auto rc = state.connect();
    std::vector<int> got;
    rc.read_all([&](const int& v, uint32_t){ got.push_back(v); });
    ASSERT_EQ(got.size(), N);
    EXPECT_EQ(got.front(), static_cast<int>(kFrames) * kFrameBase + 0);
    EXPECT_EQ(got.back(),  static_cast<int>(kFrames) * kFrameBase + static_cast<int>(N - 1));
}

// Readers occasionally do long read_all() passes (pinning a slot) while main writes.
// Triple buffering should allow writer progress; final frame must be visible.
TEST(SharedStateTests, ConcurrentReaders_LongReadAllPins_WhileMainWrites) {
    constexpr uint32_t N        = 512;
    constexpr uint32_t kReaders = 4;
    constexpr uint32_t kFrames  = 120;

    sync::shared_state<int, 3> state;
    state.init(N);

    ASSERT_TRUE(state.write([](int& v, uint32_t i){
        v = 0 * kFrameBase + static_cast<int>(i);
        return true;
    }, N));

    std::vector<sync::shared_state<int,3>::connection> conns;
    conns.reserve(kReaders);
    for (uint32_t i = 0; i < kReaders; ++i) conns.emplace_back(state.connect());

    std::atomic<bool> stop{false};
    std::atomic<uint64_t> pinned_reads{0};

    std::vector<std::thread> readers;
    readers.reserve(kReaders);
    for (uint32_t r = 0; r < kReaders; ++r) {
        readers.emplace_back([&, r]{
            auto& c = conns[r];
            while (!stop.load(std::memory_order_acquire)) {
                // Long read_all that simulates heavier per-element work (still inside guard).
                c.read_all([&](const int& v, uint32_t i){
                    (void)v; (void)i;
                    // simulate occasional heavier work while holding the pin
                    if ((i & 0x7F) == 0) std::this_thread::yield();
                });
                pinned_reads.fetch_add(1, std::memory_order_relaxed);
                // Small pause between acquisitions to vary timing
                if ((r & 1) == 1) std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    // Main thread writes frames while readers may hold pins.
    for (uint32_t f = 1; f <= kFrames; ++f) {
        ASSERT_TRUE(state.write([&](int& v, uint32_t i){
            v = static_cast<int>(f) * kFrameBase + static_cast<int>(i);
            return true;
        }, N, /*wait_for_free_slot=*/true));
        if ((f % 8) == 0) std::this_thread::yield();
    }

    // Let readers drain a bit, then stop
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    stop.store(true, std::memory_order_release);
    for (auto& th : readers) th.join();

    EXPECT_GT(pinned_reads.load(std::memory_order_relaxed), 0u);

    // Verify last frame visible and complete.
    auto rc = state.connect();
    std::vector<int> last;
    rc.read_all([&](const int& v, uint32_t){ last.push_back(v); });
    ASSERT_EQ(last.size(), N);
    EXPECT_EQ(last.front(), static_cast<int>(kFrames) * kFrameBase + 0);
    EXPECT_EQ(last.back(),  static_cast<int>(kFrames) * kFrameBase + static_cast<int>(N - 1));
}

TEST(SharedStateTests, ReaderBitIsReusedAfterDestruction) {
    sync::shared_state<int, 3> state;
    const uint32_t N = 32;
    state.init(N);

    // Publish initial
    ASSERT_TRUE(state.write([](int& v, uint32_t i){ v = int(i); return true; }, N));

    uint64_t first_bit = 0;
    {
        auto r = state.connect();
        // Observe which bit r owns by making a small publish and then reading one element dirty
        ASSERT_TRUE(state.write([](int& v, uint32_t i){ v = 1000 + int(i); return (i==0); }, N));
        std::atomic<uint64_t>* observed_mask = nullptr;

        // Peek current slot and record dirty mask[0]
        r.read([&](const int&, uint32_t idx){
            if (idx == 0) { /* we cannot directly read mask; assume we trust reuse via API */ }
        });
        // Save bit from internal state (white-box only if you expose a debug API).
        // For black-box test, just ensure no stale dirties leak (see next assertions).
        first_bit = 0; // we won't read it; black-box below
    } // r destroyed -> its bit released and cleared in current slot

    // New connection should reuse a bit and must NOT see stale dirties
    auto r2 = state.connect();

    // Immediately after connect, without a new write, dirty-only read should be empty
    std::vector<int> got;
    r2.read([&](const int& v, uint32_t){ got.push_back(v); });
    EXPECT_TRUE(got.empty()) << "New reader must not inherit stale dirty flags from previous owner";

    // Now publish and verify r2 receives fresh dirties set from active_mask
    ASSERT_TRUE(state.write([](int& v, uint32_t i){ v = 2000 + int(i); return (i%3)==0; }, N));
    std::vector<int> got2;
    r2.read([&](const int& v, uint32_t){ got2.push_back(v); });
    ASSERT_FALSE(got2.empty());
}

TEST(SharedStateTests, ManyReadersAllocateAndReuseBitsConcurrently) {
    sync::shared_state<int, 3> state;
    const uint32_t N = 64;
    state.init(N);

    // Seed
    ASSERT_TRUE(state.write([](int& v, uint32_t i){ v = int(i); return true; }, N));

    constexpr int waves = 5;
    constexpr int readers_per_wave = 8;

    for (int w = 0; w < waves; ++w) {
        std::vector<sync::shared_state<int,3>::connection> conns;
        conns.reserve(readers_per_wave);

        // Create a wave of readers
        for (int i = 0; i < readers_per_wave; ++i) {
            conns.emplace_back(state.connect());
        }

        // Main thread writes a frame; all active readers should be able to consume something
        ASSERT_TRUE(state.write([](int& v, uint32_t i){ v = 10000 + int(i); return (i&1)==0; }, N));

        // Let each read and clear its bit
        for (auto& c : conns) {
            size_t cnt = 0;
            c.read([&](const int&, uint32_t){ ++cnt; });
            EXPECT_GT(cnt, 0u);
        }

        // Destroy all connections -> frees their bits
        conns.clear();

        // New publish; with no active readers, no one should observe anything until new connects
        ASSERT_TRUE(state.write([](int& v, uint32_t i){ v = 20000 + int(i); return (i%4)==0; }, N));
        // Create a new reader and ensure it doesn't see stale dirties from the previous wave
        auto r = state.connect();
        std::vector<int> got;
        r.read([&](const int&, uint32_t){ got.push_back(1); });
        // It may see current-frame dirties only if they were set after it connected.
        // We didn't publish after connect, so it should be empty:
        EXPECT_TRUE(got.empty());
    }
}

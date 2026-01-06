#include <stdafx.h>

#include <common/sync/commands_queue.h>

using namespace tjs::common::sync;

namespace tjs::common::sync::_algo_versions
{
    namespace v1 {
        template <typename commands_set>
        class commands_queue
        {
        public:
            commands_queue() = default;


            commands_queue& operator = (const commands_queue& other)
            {
                if (&other == this)
                {
                    return *this;
                }

                std::scoped_lock lk(_mutex, other._mutex);
                std::ranges::copy(other._buffer.begin(), other._buffer.end(), _buffer.begin());

                return *this;
            }

            template <typename _cmd>
                requires std::is_trivially_copyable_v<std::remove_reference_t<_cmd>>
            void add_command(_cmd&& cmd) {
                using U = std::remove_reference_t<_cmd>;
                static_assert(std::is_constructible_v<commands_set, _cmd>,
                    "_cmd must be an alternative of commands_set (or constructible into it)");

                std::unique_lock lk(_mutex);
                _buffer.emplace_back(std::forward<_cmd>(cmd));
            }

            void drain_to(std::vector<commands_set>& out) {
                std::unique_lock lk(_mutex);
                out.swap(_buffer);
            }

            void reserve(size_t n) {
                std::unique_lock lk(_mutex);
                _buffer.reserve(n);
            }

            // Get first command
            [[nodiscard]] std::vector<commands_set> get_commands()
            {
                std::vector<commands_set> result;
                {
                    std::unique_lock lk(_mutex);
                    result.swap(_buffer);
                }
                return result;
            }

        private:
            // TODO{threaded}: atomics, lock-free?
            std::vector<commands_set> _buffer;
            mutable std::mutex _mutex;
        };
    } // v1
} // tjs::common::sync::_algo_versions

namespace {

struct open_map {
    uint32_t map_id;
    uint32_t seed;
};

struct clear_state {
    uint32_t flags;
};

using commands_set_t = std::variant<open_map, clear_state>;

static inline open_map make_open_map(uint32_t i) {
    return open_map{ i, i * 1664525u + 1013904223u };
}

static inline clear_state make_clear_state(uint32_t i) {
    return clear_state{ i };
}

// ------------------------------
// Single-thread: add_command only
// ------------------------------
template <typename queue_t>
static void BM_commands_queue_add_command_1thread(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));

    queue_t q;
    q.reserve(static_cast<size_t>(n));

    uint32_t x = 1;
    for (auto _ : state) {
        for (int i = 0; i < n; ++i) {
            if ((i & 1) == 0) q.add_command(make_open_map(x++));   // rvalue
            else              q.add_command(make_clear_state(x++)); // rvalue
        }
        // prevent unbounded growth
        auto drained = q.get_commands();
        benchmark::DoNotOptimize(drained);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * n);
}

// -------------------------------------
// Multi-thread: add_command only (MT add)
// Each benchmark thread pushes N commands.
// -------------------------------------

static std::atomic<std::barrier<>*> g_barrier_ptr{nullptr};
static std::atomic<int> g_barrier_parties{0};

static std::barrier<>* get_or_create_barrier(int parties, int thread_index) {
    // Fast path
    if (g_barrier_parties.load(std::memory_order_acquire) == parties) {
        return g_barrier_ptr.load(std::memory_order_relaxed);
    }

    if (thread_index == 0) {
        // Create a new one for this parties count
        // (We leak it; benchmark processes are short-lived.)
        auto* nb = new std::barrier<>(static_cast<std::ptrdiff_t>(parties));
        g_barrier_ptr.store(nb, std::memory_order_relaxed);
        g_barrier_parties.store(parties, std::memory_order_release);
        return nb;
    }

    // Wait until thread 0 publishes a matching barrier
    for (;;) {
        int p = g_barrier_parties.load(std::memory_order_acquire);
        if (p == parties)
        {
            return g_barrier_ptr.load(std::memory_order_relaxed);
        }
        std::this_thread::yield();
    }
}

template <typename queue_t>
static void BM_commands_queue_add_command_MT(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));

    static queue_t q; // shared across threads (mutex inside)
    
    if (state.thread_index() == 0) {
        q = queue_t{};
        q.reserve(static_cast<size_t>(n) * static_cast<size_t>(state.threads()));
        
    }
    benchmark::ClobberMemory();
    std::barrier<>* b = get_or_create_barrier(state.threads(), state.thread_index());

    b->arrive_and_wait();

    uint32_t base = static_cast<uint32_t>(state.thread_index()) * 1000000u + 1u;

    for (auto _ : state) {
        for (int i = 0; i < n; ++i) {
            uint32_t v = base + static_cast<uint32_t>(i);
            if ((i & 1) == 0) q.add_command(make_open_map(v));
            else              q.add_command(make_clear_state(v));
        }
    }

    b->arrive_and_wait();
    if (state.thread_index() == 0) {
        auto drained = q.get_commands();
        benchmark::DoNotOptimize(drained);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * n * state.threads());
}

// -----------------------------------------
// Single-thread: get_commands only (drain cost)
// Fill N commands, then drain each iteration.
// -----------------------------------------
template <typename queue_t>
static void BM_commands_queue_get_commands_1thread(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));

    queue_t q;
    q.reserve(static_cast<size_t>(n));

    uint32_t x = 1;
    for (auto _ : state) {
        // fill
        for (int i = 0; i < n; ++i) {
            if ((i & 1) == 0) q.add_command(make_open_map(x++));
            else              q.add_command(make_clear_state(x++));
        }

        // drain
        auto out = q.get_commands();
        benchmark::DoNotOptimize(out);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * n);
}

// ---------------------------------------------------------
// Multi-thread: mixed workload
// Thread 0: drains (get_commands) repeatedly
// Other threads: continuously add_command
// Measures "drain under concurrent producers" behavior.
// ---------------------------------------------------------
template <typename queue_t>
static void BM_commands_queue_get_commands_MT(benchmark::State& state) {
    const int per_producer = static_cast<int>(state.range(0));

    static queue_t q;
    static std::vector<commands_set_t> out; // reused only by consumer thread

    if (state.thread_index() == 0) {
        q = queue_t{};
        const size_t producers = (state.threads() > 1) ? (static_cast<size_t>(state.threads() - 1)) : 0u;
        q.reserve(std::max<size_t>(1u, producers) * static_cast<size_t>(per_producer));
        out.clear();
        out.reserve(std::max<size_t>(1u, producers) * static_cast<size_t>(per_producer));
    }

    std::barrier<>* b = get_or_create_barrier(state.threads(), state.thread_index());

    benchmark::ClobberMemory();
    b->arrive_and_wait();

    if (state.thread_index() == 0) {
        // Consumer: drain each iteration
        for (auto _ : state) {
            out.clear();
            q.drain_to(out);
            benchmark::DoNotOptimize(out);
            benchmark::ClobberMemory();
        }
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    } else {
        // Producers: add per_producer commands each iteration
        uint32_t base = static_cast<uint32_t>(state.thread_index()) * 1000000u + 1u;
        for (auto _ : state) {
            for (int i = 0; i < per_producer; ++i) {
                uint32_t v = base + static_cast<uint32_t>(i);
                if ((i & 1) == 0) q.add_command(make_open_map(v));
                else              q.add_command(make_clear_state(v));
            }
        }
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * per_producer);
    }
}

} // namespace

// ---------------
// Registrations
// ---------------
BENCHMARK(BM_commands_queue_add_command_1thread<_algo_versions::v1::commands_queue<commands_set_t>>)
    ->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);

BENCHMARK(BM_commands_queue_add_command_MT<_algo_versions::v1::commands_queue<commands_set_t>>)
    ->Arg(64)->Arg(256)->Arg(1024)
    ->ThreadRange(4, 8);

BENCHMARK(BM_commands_queue_get_commands_1thread<_algo_versions::v1::commands_queue<commands_set_t>>)
    ->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);

BENCHMARK(BM_commands_queue_get_commands_MT<_algo_versions::v1::commands_queue<commands_set_t>>)
    ->Arg(64)->Arg(256)->Arg(1024)
    ->ThreadRange(4, 8);

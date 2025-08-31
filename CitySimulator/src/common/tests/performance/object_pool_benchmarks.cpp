#include <stdafx.h>

#include <common/object_pool.h>


namespace _test {
    template <class T>
    class object_pool {
    public:
        using index_t = std::uint32_t;
        static constexpr index_t npos = std::numeric_limits<index_t>::max();

        object_pool() = default;
        explicit object_pool(std::size_t reserve_slots) { reserve(reserve_slots); }

        // Create a T and return its slot index (stable for the life of the object).
        template <class... Args>
        index_t create(Args&&... args) {
            index_t idx;
            if (!free_list_.empty()) {
                idx = free_list_.back();
                free_list_.pop_back();
                storage_[idx] = std::make_unique<T>(std::forward<Args>(args)...);
            } else {
                idx = static_cast<index_t>(storage_.size());
                storage_.push_back(std::make_unique<T>(std::forward<Args>(args)...));
            }
            ++alive_count_;
            return idx;
        }

        // Destroy the object at slot idx (no-op if already empty or out of range).
        void destroy(index_t idx) noexcept {
            if (idx >= storage_.size()) return;
            if (!storage_[idx]) return;
            storage_[idx].reset();
            free_list_.push_back(idx);
            --alive_count_;
        }

        // Get raw pointer (nullptr if idx invalid or freed).
        T* get(index_t idx) noexcept {
            return (idx < storage_.size()) ? storage_[idx].get() : nullptr;
        }
        const T* get(index_t idx) const noexcept {
            return (idx < storage_.size()) ? storage_[idx].get() : nullptr;
        }

        // Iterate live objects.
        template <class Fn>
        void for_each(Fn&& fn) {
            for (auto& p : storage_) if (p) fn(*p);
        }

        // Pre-create empty slots. Pointers for existing objects stay valid.
        void reserve(std::size_t slots) {
            if (slots <= storage_.size()) return;
            const std::size_t old = storage_.size();
            storage_.resize(slots); // add nullptr slots
            for (std::size_t i = slots; i-- > old; ) {
                free_list_.push_back(static_cast<index_t>(i));
            }
        }

        void clear() {
            storage_.clear();
            free_list_.clear();
            alive_count_ = 0;
        }

        std::size_t size() const noexcept { return alive_count_; }     // live objects
        std::size_t capacity() const noexcept { return storage_.size(); } // total slots

    private:
        std::vector<std::unique_ptr<T>> storage_; // nullptr == free slot
        std::vector<index_t> free_list_;          // indices of free slots (LIFO)
        std::size_t alive_count_ = 0;
    };
}


// Trivial, cache-line sized object (focus on alloc/free paths)
struct alignas(64) trivial64_t {
    uint64_t data[8];
};

// Small POD
struct small_pod_t {
    int a{};
    int b{};
    double c{};
};

// Non-trivial (constructor / destructor do heap work via std::string)
struct non_trivial_t {
    std::string s;
    non_trivial_t() : s(32, 'x') {}
};


// -------------------------------------------------------------
// Global/shared pools for benchmarks
// (shared across threads in multi-threaded runs)
// -------------------------------------------------------------
using tjs::common::ObjectPool;

static _test::object_pool<trivial64_t> g_vector_trivial;

static ObjectPool<trivial64_t> g_pool_trivial;
static ObjectPool<small_pod_t>  g_pool_small;
static ObjectPool<non_trivial_t> g_pool_nontrivial;

// Utility: per-thread RNG
static thread_local std::mt19937_64 tls_rng{std::random_device{}()};

// -------------------------------------------------------------
// Baseline: new/delete (trivial64_t)
// -------------------------------------------------------------
static void bm_new_delete_trivial64(benchmark::State& state) {
    for (auto _ : state) {
        auto* p = new trivial64_t();
        benchmark::DoNotOptimize(p);
        delete p;
        benchmark::ClobberMemory();
    }
}

// -------------------------------------------------------------
// Pool: acquire → release (trivial64_t)
// -------------------------------------------------------------
static void bm_vector_pool_trivial64(benchmark::State& state) {
    for (auto _ : state) {
        auto idx = g_vector_trivial.create();
        auto pp = g_vector_trivial.get(idx);
        benchmark::DoNotOptimize(pp);
        g_vector_trivial.destroy(idx);
        benchmark::ClobberMemory();
    }
}


// -------------------------------------------------------------
// Pool: acquire → release (trivial64_t)
// -------------------------------------------------------------
static void bm_pool_acquire_release_trivial64(benchmark::State& state) {
    for (auto _ : state) {
        auto pp = g_pool_trivial.acquire();
        benchmark::DoNotOptimize(pp.get());
        g_pool_trivial.release(pp);
        benchmark::ClobberMemory();
    }
}

// Trivial 1 thread
BENCHMARK(bm_new_delete_trivial64)->Name("baseline/new_delete/trivial64")->Threads(1);
BENCHMARK(bm_vector_pool_trivial64)->Name("vector_pool/new_delete/trivial64")->Threads(1);
BENCHMARK(bm_pool_acquire_release_trivial64)->Name("pool/acquire_release/trivial64")->Threads(1);

// Half the number of hardware threads
BENCHMARK(bm_new_delete_trivial64)->Name("baseline/new_delete/trivial64/threads")
    ->Threads(std::max(1, (int)std::thread::hardware_concurrency()/2));
BENCHMARK(bm_pool_acquire_release_trivial64)->Name("pool/acquire_release/trivial64/threads")
    ->Threads(std::max(1, (int)std::thread::hardware_concurrency()/2));

// 8 threads
BENCHMARK(bm_new_delete_trivial64)->Threads(8);
BENCHMARK(bm_pool_acquire_release_trivial64)->Threads(8);
// 16 threads
BENCHMARK(bm_new_delete_trivial64)->Threads(16);
BENCHMARK(bm_pool_acquire_release_trivial64)->Threads(16);


// -------------------------------------------------------------
// Pool vs new/delete for a small POD
// -------------------------------------------------------------
static void bm_new_delete_small_pod(benchmark::State& state) {
    for (auto _ : state) {
        auto* p = new small_pod_t();
        benchmark::DoNotOptimize(p);
        delete p;
    }
}

static void bm_pool_small_pod(benchmark::State& state) {
    for (auto _ : state) {
        auto pp = g_pool_small.acquire();
        benchmark::DoNotOptimize(pp.get());
        g_pool_small.release(pp);
    }
}

BENCHMARK(bm_new_delete_small_pod)->Name("baseline/new_delete/small_pod")->Threads(1);
BENCHMARK(bm_pool_small_pod)->Name("pool/acquire_release/small_pod")->Threads(1);


// -------------------------------------------------------------
// Non-trivial type (constructor/destructor cost dominates)
// This shows the *upper bound* where allocator overhead is hidden
// -------------------------------------------------------------
static void bm_new_delete_nontrivial(benchmark::State& state) {
    for (auto _ : state) {
        auto* p = new non_trivial_t();
        benchmark::DoNotOptimize(p);
        delete p;
    }
}

static void bm_pool_nontrivial(benchmark::State& state) {
    for (auto _ : state) {
        auto pp = g_pool_nontrivial.acquire();
        benchmark::DoNotOptimize(pp.get());
        g_pool_nontrivial.release(pp);
    }
}

BENCHMARK(bm_new_delete_nontrivial)->Name("baseline/new_delete/non_trivial")->Threads(1);
BENCHMARK(bm_pool_nontrivial)->Name("pool/acquire_release/non_trivial")->Threads(1);

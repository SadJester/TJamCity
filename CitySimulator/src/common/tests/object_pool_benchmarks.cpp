#include <stdafx.h>

#include <common/object_pool.h>


namespace _test {
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

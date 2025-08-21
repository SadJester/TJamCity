#include <stdafx.h>
#include <memory>

#include <common/object_pool.h>

using tjs::common::ObjectPool;

// -------------------------------------------------------------
// Test payload type: trivial POD with one field to touch.
// 64B aligned to model cache-line-friendly vehicle state.
// -------------------------------------------------------------
struct alignas(64) simple_t {
    uint64_t v{0};
};

// trivial op: read-modify-write (so the compiler can't dead-code it)
inline void trivial_op(simple_t* p) noexcept {
    // add a constant, xor, keep it cheap
    p->v += 0x9E3779B97F4A7C15ull;
    p->v ^= (p->v >> 13);
    benchmark::DoNotOptimize(p);
}

// -------------------------------------------------------------
// 1) BASELINE (new/delete) — COLD by nature on each run
// -------------------------------------------------------------
static void bm_baseline_create_iter_destroy(benchmark::State& state) {
    const std::size_t n = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        // create
        std::vector<simple_t*> buf;
        buf.reserve(n);
        for (std::size_t i = 0; i < n; ++i) {
            auto* p = new simple_t();
            benchmark::DoNotOptimize(p);
            buf.push_back(p);
        }

        // iterate + trivial op
        for (auto* p : buf) {
            trivial_op(p);
        }

        // destroy
        for (auto* p : buf) {
            delete p;
        }

        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(bm_baseline_create_iter_destroy)
    ->Name("baseline/new_delete/create_iter_destroy/COLD")
    ->Arg(1 << 10)   // 1k
    ->Arg(1 << 15)   // 32k
    ->Arg(1 << 20);  // 1M

// -------------------------------------------------------------
// 2) POOL — COLD: build a fresh pool inside timed region, destroy it.
// Includes slab alloc/free and per-object construct/destruct.
// -------------------------------------------------------------
static void bm_pool_create_iter_destroy_COLD(benchmark::State& state) {
    const std::size_t n = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        // everything in timed region, including pool ctor/dtor
        {
            ObjectPool<simple_t> pool;          // cold each iteration
            std::vector<typename ObjectPool<simple_t>::pooled_ptr> buf;
            buf.reserve(n);

            // create (acquire)
            for (std::size_t i = 0; i < n; ++i) {
                auto pp = pool.acquire();
                benchmark::DoNotOptimize(pp.get());
                buf.push_back(pp);
            }

            // iterate + trivial op
            for (auto& pp : buf) {
                trivial_op(pp.get());
            }

            // destroy (release)
            for (auto& pp : buf) {
                pool.release(pp);
            }

            // pool dtor runs here (frees slabs); included in timing
        }

        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(bm_pool_create_iter_destroy_COLD)
    ->Name("pool/create_iter_destroy/COLD")
    ->Arg(1 << 10)
    ->Arg(1 << 15)
    ->Arg(1 << 20);

// -------------------------------------------------------------
// 3) POOL — WARM: pool persists across iterations, pre-reserved.
// Measures acquire+iterate+release cost without slab (re)alloc.
// -------------------------------------------------------------

class warm_pool_fixture : public benchmark::Fixture {
public:
    using pool_t = tjs::common::ObjectPool<simple_t>;
    using pp_t   = typename pool_t::pooled_ptr;

    void SetUp(const ::benchmark::State& state) override {
        n_ = static_cast<std::size_t>(state.range(0));
        // pre-reserve once per benchmark *run*, not per iteration
        pool_.reserve(n_);
        buf_.reserve(n_);
    }

    void TearDown(const ::benchmark::State&) override {
        // make sure nothing is left live, even if an iteration aborted
        for (auto& p : buf_) {
            if (p) {
                pool_.release(p);
            }
        }
        buf_.clear();
    }

protected:
    pool_t pool_;
    std::vector<pp_t> buf_;
    std::size_t n_{};
};

// Single-thread WARM: acquire N → iterate → release N (steady state)
BENCHMARK_DEFINE_F(warm_pool_fixture, create_iter_destroy_WARM)(benchmark::State& state) {
    const std::size_t n = n_;
    for (auto _ : state) {
        buf_.clear();
        // acquire
        for (std::size_t i = 0; i < n; ++i) {
            auto pp = pool_.acquire();
            benchmark::DoNotOptimize(pp.get());
            buf_.push_back(pp);
        }
        // iterate trivial op
        for (auto& pp : buf_) {
            trivial_op(pp.get());
        }
        // release
        for (auto& pp : buf_) {
            pool_.release(pp);
        }
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}

BENCHMARK_REGISTER_F(warm_pool_fixture, create_iter_destroy_WARM)
    ->Name("pool/create_iter_destroy/WARM")
    ->Arg(1 << 10)
    ->Arg(1 << 15)
    ->Arg(1 << 20);

// -------------------------------------------------------------
// Optional: contiguous upper bound (vector<simple_t>) to see
// best-case iteration when construction is amortized at once.
// Not directly comparable to new/delete but useful as a ceiling.
// -------------------------------------------------------------
static void bm_vector_contiguous_create_iter_destroy(benchmark::State& state) {
    const std::size_t n = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        // create contiguous
        std::vector<simple_t> v;
        v.resize(n);

        // iterate + trivial op
        for (auto& x : v) {
            trivial_op(&x);
        }

        // destroy happens when v goes out of scope
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(bm_vector_contiguous_create_iter_destroy)
    ->Name("upper_bound/vector_contiguous/create_iter_destroy")
    ->Arg(1 << 10)
    ->Arg(1 << 15)
    ->Arg(1 << 20);

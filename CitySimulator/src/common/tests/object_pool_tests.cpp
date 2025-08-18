// ObjectPool tests (GoogleTest)
// Assumptions:
//  - Class name: ObjectPool<_T, _BlockSize, _TLSCacheSize>
//  - API pieces used below exist:
//      .block_count(), .capacity(), .flush_thread_cache()
//      pooled_ptr { T* ptr; uint32_t idx; }
//      acquire(...), release(pooled_ptr), release_by_index(uint32_t)
//  - Pool uses std::construct_at / std::destroy_at internally
//  - Blocks are 64B aligned (as in your aligned operator new[])

#include <stdafx.h>
#include <common/object_pool.h>

#include <barrier>

using namespace tjs::common;

// ---- test object
struct alignas(16) test_obj {
	static inline std::atomic<int> live { 0 };
	static inline std::atomic<int> ctor_calls { 0 };
	static inline std::atomic<int> dtor_calls { 0 };

	uint64_t id = 0;
	char pad[48] {}; // make it ~64B in total

	explicit test_obj(uint64_t v = 0)
		: id(v) {
		ctor_calls.fetch_add(1, std::memory_order_relaxed);
		live.fetch_add(1, std::memory_order_relaxed);
	}
	~test_obj() {
		dtor_calls.fetch_add(1, std::memory_order_relaxed);
		live.fetch_sub(1, std::memory_order_relaxed);
	}

	static void reset_counters() {
		live.store(0);
		ctor_calls.store(0);
		dtor_calls.store(0);
	}
};

// ---- Test fixture: auto-reset counters
class object_pool_fixture : public ::testing::Test {
protected:
	void SetUp() override { test_obj::reset_counters(); }
	void TearDown() override {
		// sanity: no leaks between tests
		ASSERT_EQ(test_obj::live.load(), 0) << "Live objects leaked across tests";
		ASSERT_EQ(test_obj::ctor_calls.load(), test_obj::dtor_calls.load())
			<< "Ctor/Dtor mismatch across tests";
	}
};

// For convenience in tests we shrink BlockSize/TLSCache to exercise edge cases fast.
template<size_t BlockSize = 8, size_t TLSCache = 4>
using pool8x4_t = ObjectPool<test_obj, BlockSize, TLSCache>;

// helper: acquire N objects and return pooled_ptrs
template<size_t B, size_t C>
static std::vector<typename pool8x4_t<B, C>::pooled_ptr> acquire_n(pool8x4_t<B, C>& pool, int n) {
	using handle_t = typename pool8x4_t<B, C>::pooled_ptr;
	std::vector<handle_t> out;
	out.reserve(n);
	for (int i = 0; i < n; ++i) {
		out.push_back(pool.acquire(i + 1));
	}
	return out;
}

// 1) Construction of objects that need only one allocation
TEST_F(object_pool_fixture, single_block_construction_only_one_allocation) {
	pool8x4_t<> pool; // BlockSize=8, TLS=4
	auto handles = acquire_n(pool, 8);
	EXPECT_EQ(test_obj::live.load(), 8);
	EXPECT_EQ(pool.block_count(), 1u);
	EXPECT_EQ(pool.capacity(), 8u);

	// first pointer should be 64B aligned if your pool aligns blocks
	auto base = reinterpret_cast<std::uintptr_t>(handles[0].ptr);
	EXPECT_EQ(base % 64u, 0u);

	for (auto& h : handles) {
		pool.release(h);
	}
}

// 2) Construct-delete-construct -> expect the first address (slot) to be reused
TEST_F(object_pool_fixture, construct_delete_construct_reuses_first_address) {
	pool8x4_t<> pool;
	auto h1 = pool.acquire(111);
	auto* addr1 = h1.ptr;
	auto idx1 = h1.idx;

	pool.release(h1);
	EXPECT_EQ(test_obj::live.load(), 0);

	auto h2 = pool.acquire(222);
	EXPECT_EQ(h2.ptr, addr1);
	EXPECT_EQ(h2.idx, idx1);

	pool.release(h2);
}

// 3a) Multithreaded use that does NOT exceed TLS cache
TEST_F(object_pool_fixture, multithread_below_tls_cache) {
	pool8x4_t<> pool;

	constexpr int threads = 8;
	constexpr int per_thread_ops = 4; // == TLS cache size
	std::barrier sync(threads);

	std::vector<std::thread> ws;
	ws.reserve(threads);
	for (int t = 0; t < threads; ++t) {
		ws.emplace_back([&, t] {
			sync.arrive_and_wait();
			std::vector<typename pool8x4_t<>::pooled_ptr> local;
			local.reserve(per_thread_ops);
			for (int i = 0; i < per_thread_ops; ++i) {
				local.push_back(pool.acquire((t << 16) + i));
			}
			for (auto& h : local) {
				pool.release(h);
			}
		});
	}
	for (auto& th : ws) {
		th.join();
	}

	EXPECT_EQ(test_obj::live.load(), 0);
	EXPECT_LE(pool.block_count(), static_cast<size_t>(
									  (threads * per_thread_ops + 7) / 8));
}

// 3b) Multithreaded use that DOES exceed TLS cache (forces spills/refills)
TEST_F(object_pool_fixture, multithread_exceed_tls_cache_spill_and_refill) {
	pool8x4_t<> pool;

	constexpr int threads = 8;
	constexpr int per_thread_ops = 4 * 8; // 8x TLS -> spills
	std::barrier sync(threads);

	std::vector<std::thread> ws;
	ws.reserve(threads);
	for (int t = 0; t < threads; ++t) {
		ws.emplace_back([&, t] {
			sync.arrive_and_wait();
			// Round A: acquire half then release
			std::vector<typename pool8x4_t<>::pooled_ptr> local;
			local.reserve(per_thread_ops / 2);
			for (int i = 0; i < per_thread_ops / 2; ++i) {
				local.push_back(pool.acquire((t << 16) + i));
			}
			for (auto& h : local) {
				pool.release(h);
			}
			local.clear();
			// Round B: many short-lived acquire/release to churn TLS/global
			for (int i = 0; i < per_thread_ops; ++i) {
				auto h = pool.acquire((t << 16) + i + 10000);
				pool.release(h);
			}
		});
	}
	for (auto& th : ws) {
		th.join();
	}

	EXPECT_EQ(test_obj::live.load(), 0);
	EXPECT_GE(pool.block_count(), 1u);
}

// 4) Pool growth across multiple blocks
TEST_F(object_pool_fixture, grows_to_multiple_blocks) {
	pool8x4_t<> pool;
	const int total = 3 * 8 + 1; // > three blocks
	auto hs = acquire_n(pool, total);

	EXPECT_EQ(pool.block_count(), 4u);
	EXPECT_EQ(pool.capacity(), 32u);
	EXPECT_EQ(test_obj::live.load(), total);

	for (auto& h : hs) {
		pool.release(h);
	}
}

// 6) Dtors run at pool destruction for unreleased objects
TEST_F(object_pool_fixture, dtors_called_on_pool_destruction) {
	{
		pool8x4_t<> pool;
		auto a = pool.acquire(1);
		auto b = pool.acquire(2);
		auto c = pool.acquire(3);
		(void)a;
		(void)b;
		(void)c;
		EXPECT_EQ(test_obj::live.load(), 3);
	}
	// TearDown will assert ctor==dtor and live==0
}

// 7) Reserve exact-fit and next acquire triggers a new block
TEST_F(object_pool_fixture, reserve_exact_fit_then_grow) {
	pool8x4_t<> pool;
	// reserve one block explicitly if your pool supports reserve()
	pool.reserve(8);
	auto hs = acquire_n(pool, 8);
	EXPECT_EQ(pool.block_count(), 1u);

	// next one should force a new block
	auto extra = pool.acquire(999);
	EXPECT_EQ(pool.block_count(), 2u);

	for (auto& h : hs) {
		pool.release(h);
	}
	pool.release(extra);
}
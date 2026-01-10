#include <stdafx.h>

#include <common/sync/message.h>

using namespace tjs::common;

namespace {
	struct small_trivial {
		int v;
		small_trivial(int x = 0)
			: v(x) {}
		bool operator==(const small_trivial& o) const { return v == o.v; }
	};

	// Force a clearly heap-allocated case: big and over-aligned
	struct alignas(64) big_aligned {
		int x;
		char pad[256]; // large to exceed small buffer
		explicit big_aligned(int x_)
			: x(x_)
			, pad {} {}
	};
	static_assert(alignof(big_aligned) >= 64, "expected over-aligned type");

	struct DtorCounted {
		static inline std::atomic<int> live { 0 };
		static inline std::atomic<int> destroyed { 0 };
		int v {};
		explicit DtorCounted(int vv = 0)
			: v(vv) { ++live; }
		DtorCounted(const DtorCounted&) = delete;
		DtorCounted& operator=(const DtorCounted&) = delete;
		DtorCounted(DtorCounted&& o) noexcept
			: v(o.v) {
			++live;
			o.v = 0;
		}
		DtorCounted& operator=(DtorCounted&& o) noexcept {
			if (this != &o) {
				v = o.v;
			}
			return *this;
		}
		~DtorCounted() {
			++destroyed;
			--live;
		}
	};

	// Move-only payload to verify perfect forwarding
	struct MoveOnly {
		std::unique_ptr<int> p;
		explicit MoveOnly(std::unique_ptr<int>&& q)
			: p(std::move(q)) {}
		MoveOnly(MoveOnly&&) noexcept = default;
		MoveOnly& operator=(MoveOnly&&) noexcept = default;
		MoveOnly(const MoveOnly&) = delete;
		MoveOnly& operator=(const MoveOnly&) = delete;
	};

	// Slightly large object (to force heap) — size chosen in tests vs buffer size
	template<std::size_t N>
	struct Big {
		char bytes[N];
		int mark {};
		explicit Big(int m = 0)
			: bytes {}
			, mark(m) {}
	};

	enum class msg_kind : char {
		A = 1,
		B = 2,
		C = 3,
		D = 4
	};
} // namespace

TEST(SyncMessageTests, SmokeTest) {
	enum class TestEnum { One,
		Two,
		Three };
	struct Test {
		int a[10];
		Test() {
			std::fill_n(a, 10, 42);
		}
		Test(int val) {
			std::fill_n(a, 10, val);
		}
	};

	using test_message = sync::message<TestEnum, 8>;

	test_message m1(TestEnum::One);
	EXPECT_FALSE(m1.is_empty());

	test_message m2(TestEnum::Two, Test {});
	EXPECT_EQ(m2.get<Test>().a[0], 42);

	test_message m3(TestEnum::Three, 10);
	EXPECT_EQ(m3.get<int>(), 10);

	test_message m4(TestEnum::Two);
	m4.emplace<Test>(100);
	EXPECT_EQ(m4.get<Test>().a[0], 100);
}

TEST(SyncMessageTests, Inline_Int_And_ConstGet) {
	enum class kind { k };
	using msg_t = sync::message<kind, /*buffer_size*/ 32>;

	const msg_t m(kind::k, 123);
	// const get<T>()
	EXPECT_EQ(m.get<int>(), 123);

	msg_t m2(kind::k, -5);
	EXPECT_EQ(m2.get<int>(), -5);
}

TEST(SyncMessageTests, Inline_SmallTrivial_Type) {
	enum class kind { k };
	using msg_t = sync::message<kind, 32>;

	msg_t m(kind::k, small_trivial { 7 });
	auto& s = m.get<small_trivial>();
	EXPECT_EQ(s.v, 7);
}

TEST(SyncMessageTests, Heap_OverAligned_Object_AddressIsProperlyAligned) {
	enum class kind { k };
	// Make inline buffer intentionally small to force heap path
	using msg_t = sync::message<kind, 16>;

	msg_t m(kind::k, big_aligned { 42 });

	const big_aligned& ref = m.get<big_aligned>();
	auto addr = reinterpret_cast<std::uintptr_t>(&ref);
	// Check 64-byte alignment (or better)
	EXPECT_EQ(addr % 64, 0u);
	EXPECT_EQ(ref.x, 42);
}

TEST(SyncMessageTests, MoveCtor_Inline_CopySemanticsWork) {
	enum class kind { a,
		b };
	using msg_t = sync::message<kind, 64>;

	msg_t m1(kind::a, small_trivial { 123 });
	msg_t m2(std::move(m1)); // move-construct

	EXPECT_EQ(m2.get<small_trivial>().v, 123);
	// m1 is in a moved-from state; we don`t dereference it
}

TEST(SyncMessageTests, MoveAssign_Inline_CopySemanticsWork) {
	enum class kind { a,
		b };
	using msg_t = sync::message<kind, 64>;

	msg_t m1(kind::a, small_trivial { 55 });
	msg_t m2(kind::b, small_trivial { 66 });

	m2 = std::move(m1);
	EXPECT_EQ(m2.get<small_trivial>().v, 55);
}

TEST(SyncMessageTests, MoveCtor_Heap_StealsPointer) {
	enum class kind { k };
	using msg_t = sync::message<kind, 8>; // tiny buffer to force heap

	msg_t m1(kind::k, big_aligned { 777 });
	msg_t m2(std::move(m1));

	EXPECT_EQ(m2.get<big_aligned>().x, 777);
	// scope exit should not double delete
}

TEST(SyncMessageTests, MoveAssign_Heap_StealsPointer) {
	enum class kind { k };
	using msg_t = sync::message<kind, 8>; // tiny buffer to force heap

	msg_t m1(kind::k, big_aligned { 101 });
	msg_t m2(kind::k, big_aligned { 202 });

	m2 = std::move(m1);
	EXPECT_EQ(m2.get<big_aligned>().x, 101);
}

TEST(SyncMessageTests, Emplace_OnEmpty_Works_Inline) {
	enum class kind { k };
	using msg_t = sync::message<kind, 64>;

	msg_t m(kind::k);
	auto& in = m.emplace<small_trivial>(small_trivial { 314 });
	EXPECT_EQ(in.v, 314);
	EXPECT_EQ(m.get<small_trivial>().v, 314);
}

TEST(SyncMessageTests, Emplace_OnEmpty_Works_Heap) {
	enum class kind { k };
	using msg_t = sync::message<kind, 8>; // force heap for big_aligned

	msg_t m(kind::k);
	auto& ref = m.emplace<big_aligned>(big_aligned { 909 });
	EXPECT_EQ(ref.x, 909);
	EXPECT_EQ(m.get<big_aligned>().x, 909);

	auto addr = reinterpret_cast<std::uintptr_t>(&m.get<big_aligned>());
	EXPECT_EQ(addr % 64, 0u);
}

TEST(SyncMessageTests, InlineSmallObjectAndTypeUpdates) {
	// Use a small inline buffer so SmallPod fits inline
	using Msg = sync::message<msg_kind, /*buffer_size*/ 64>;

	{
		Msg m { msg_kind::A };
		auto& s = m.replace<small_trivial>(msg_kind::B, 3);
		EXPECT_EQ(m.type(), msg_kind::B);
		EXPECT_EQ(s.v, 3);

		// Replace inline with another inline type — previous should be destroyed
		auto& t = m.replace<small_trivial>(msg_kind::C, 10);
		EXPECT_EQ(m.type(), msg_kind::C);
		EXPECT_EQ(t.v, 10);

		// Accessors
		const auto& cref = m.get<small_trivial>();
		EXPECT_EQ(cref.v, 10);
	}
	// No explicit leak checks here; just compilation/runtime sanity for inline path
}

TEST(SyncMessageTests, HeapForLargeObjectAndDestructorRuns) {
	using Msg = sync::message<msg_kind, /*buffer_size*/ 64>;

	DtorCounted::live = 0;
	DtorCounted::destroyed = 0;

	{
		Msg m { msg_kind::A };

		// First place a DtorCounted inline (fits in 64)
		auto& d = m.replace<DtorCounted>(msg_kind::B, 42);
		EXPECT_EQ(d.v, 42);

		// Now force a heap allocation by using Big<256> ( > 64 )
		auto& b = m.replace<Big<256>>(msg_kind::C, 99);
		EXPECT_EQ(m.type(), msg_kind::C);
		EXPECT_EQ(b.mark, 99);

		// The DtorCounted should have been destroyed when replaced
		EXPECT_EQ(DtorCounted::live.load(), 0);
		EXPECT_EQ(DtorCounted::destroyed.load(), 1);
	}
	// On scope exit, the Big<256> should be destroyed
	// (we can't count its dtors; we just ensure no crashes and prior counters are balanced)
}

TEST(SyncMessageTests, PerfectForwardingMoveOnly) {
	using Msg = sync::message<msg_kind, /*buffer_size*/ 64>;

	Msg m { msg_kind::A };

	auto& mo = m.replace<MoveOnly>(msg_kind::D, std::make_unique<int>(123));
	ASSERT_TRUE(mo.p);
	EXPECT_EQ(*mo.p, 123);

	// Replace again with another move-only to ensure previous is destroyed correctly
	auto& mo2 = m.replace<MoveOnly>(msg_kind::B, std::make_unique<int>(456));
	ASSERT_TRUE(mo2.p);
	EXPECT_EQ(*mo2.p, 456);
}

TEST(SyncMessageTests, NoPayloadMessage) {
	enum class kind { k,
		m };
	using msg_t = sync::message<kind, 8>;

	msg_t m1(kind::k);

	ASSERT_FALSE(m1.is_empty());
	ASSERT_EQ(m1.type(), kind::k);

	m1.replace(kind::m);

	ASSERT_FALSE(m1.is_empty());
	ASSERT_EQ(m1.type(), kind::m);
}

struct PayloadWithAllowed {
	static constexpr msg_kind ALLOWED_IN_MESSAGES[] = { msg_kind::A, msg_kind::B };

	int test;
};

TEST(SyncMessageTests, MessageWithAllowedMessagesPayload) {
	using msg_t = sync::message<msg_kind, 8>;

	msg_t m1;
	m1.replace<PayloadWithAllowed>(msg_kind::A, 2);
	auto& payload = m1.get<msg_kind::A, PayloadWithAllowed>();
	ASSERT_EQ(payload.test, 2);
}

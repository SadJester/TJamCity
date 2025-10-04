#include <stdafx.h>

#include <common/sync/message.h>

using namespace tjs::common;

namespace {
    struct small_trivial {
        int v;
        small_trivial(int x = 0) : v(x) {}
        bool operator==(const small_trivial& o) const { return v == o.v; }
    };

    // Force a clearly heap-allocated case: big and over-aligned
    struct alignas(64) big_aligned {
        int x;
        char pad[256]; // large to exceed small buffer
        explicit big_aligned(int x_) : x(x_), pad{} {}
    };
    static_assert(alignof(big_aligned) >= 64, "expected over-aligned type");
}


TEST(SyncMessageTests, SmokeTest) {
    enum class TestEnum {One, Two, Three};
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
    EXPECT_EQ(&m1.get<Test>(), nullptr);

    test_message m2(TestEnum::Two, Test{});
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

    msg_t m(kind::k, small_trivial{7});
    auto& s = m.get<small_trivial>();
    EXPECT_EQ(s.v, 7);
}

TEST(SyncMessageTests, Heap_OverAligned_Object_AddressIsProperlyAligned) {
    enum class kind { k };
    // Make inline buffer intentionally small to force heap path
    using msg_t = sync::message<kind, 16>;

    msg_t m(kind::k, big_aligned{42});

    const big_aligned& ref = m.get<big_aligned>();
    auto addr = reinterpret_cast<std::uintptr_t>(&ref);
    // Check 64-byte alignment (or better)
    EXPECT_EQ(addr % 64, 0u);
    EXPECT_EQ(ref.x, 42);
}

TEST(SyncMessageTests, MoveCtor_Inline_CopySemanticsWork) {
    enum class kind { a, b };
    using msg_t = sync::message<kind, 64>;

    msg_t m1(kind::a, small_trivial{123});
    msg_t m2(std::move(m1)); // move-construct

    EXPECT_EQ(m2.get<small_trivial>().v, 123);
    // m1 is in a moved-from state; we don`t dereference it
}

TEST(SyncMessageTests, MoveAssign_Inline_CopySemanticsWork) {
    enum class kind { a, b };
    using msg_t = sync::message<kind, 64>;

    msg_t m1(kind::a, small_trivial{55});
    msg_t m2(kind::b, small_trivial{66});

    m2 = std::move(m1);
    EXPECT_EQ(m2.get<small_trivial>().v, 55);
}

TEST(SyncMessageTests, MoveCtor_Heap_StealsPointer) {
    enum class kind { k };
    using msg_t = sync::message<kind, 8>; // tiny buffer to force heap

    msg_t m1(kind::k, big_aligned{777});
    msg_t m2(std::move(m1));

    EXPECT_EQ(m2.get<big_aligned>().x, 777);
    // scope exit should not double delete
}

TEST(SyncMessageTests, MoveAssign_Heap_StealsPointer) {
    enum class kind { k };
    using msg_t = sync::message<kind, 8>; // tiny buffer to force heap

    msg_t m1(kind::k, big_aligned{101});
    msg_t m2(kind::k, big_aligned{202});

    m2 = std::move(m1);
    EXPECT_EQ(m2.get<big_aligned>().x, 101);
}

TEST(SyncMessageTests, Emplace_OnEmpty_Works_Inline) {
    enum class kind { k };
    using msg_t = sync::message<kind, 64>;

    msg_t m(kind::k);
    auto& in = m.emplace<small_trivial>(small_trivial{314});
    EXPECT_EQ(in.v, 314);
    EXPECT_EQ(m.get<small_trivial>().v, 314);
}

TEST(SyncMessageTests, Emplace_OnEmpty_Works_Heap) {
    enum class kind { k };
    using msg_t = sync::message<kind, 8>; // force heap for big_aligned

    msg_t m(kind::k);
    auto& ref = m.emplace<big_aligned>(big_aligned{909});
    EXPECT_EQ(ref.x, 909);
    EXPECT_EQ(m.get<big_aligned>().x, 909);

    auto addr = reinterpret_cast<std::uintptr_t>(&m.get<big_aligned>());
    EXPECT_EQ(addr % 64, 0u);
}

#include <stdafx.h>

#include <common/patterns/ilistener.h>

/// <summary>
/// </summary>

using namespace tjs::common;

struct event_1 {
	int v = 0;
};
struct event_2 {
	float v = 0.f;
};
using all_events = type_list<event_1, event_2>;

// ---- test listeners ----
struct listener_both : listener_crtp<listener_both, all_events> {
	void handle(const event_1& e) {
		e1_calls++;
		e1_seen.push_back(e.v);
	}
	void handle(const event_2& e) {
		e2_calls++;
		e2_seen.push_back(e.v);
	}

	int e1_calls = 0;
	int e2_calls = 0;
	std::vector<int> e1_seen;
	std::vector<float> e2_seen;
};

struct listener_only_e1 : listener_crtp<listener_only_e1, all_events> {
	void handle(const event_1& e) {
		e1_calls++;
		e1_seen.push_back(e.v);
	}

	int e1_calls = 0;
	std::vector<int> e1_seen;
};

struct listener_none : listener_crtp<listener_none, all_events> {
	// implements nothing
	int dummy = 0;
};

// -------------------- TESTS --------------------

TEST(event_bus_jump_table, delivers_to_supported_listeners) {
	event_bus<all_events> bus;
	listener_both a;
	listener_only_e1 b;

	bus.subscribe(a);
	bus.subscribe(b);

	bus.publish(event_1 { 10 });
	bus.publish(event_1 { 20 });
	bus.publish(event_2 { 1.5f });

	EXPECT_EQ(a.e1_calls, 2);
	EXPECT_EQ(a.e2_calls, 1);
	EXPECT_EQ(a.e1_seen, (std::vector<int> { 10, 20 }));
	ASSERT_EQ(a.e2_seen.size(), 1u);
	EXPECT_FLOAT_EQ(a.e2_seen[0], 1.5f);

	EXPECT_EQ(b.e1_calls, 2);
	EXPECT_EQ(b.e1_seen, (std::vector<int> { 10, 20 }));
}

TEST(event_bus_jump_table, does_not_call_for_unsupported_event) {
	event_bus<all_events> bus;
	listener_only_e1 b;

	bus.subscribe(b);

	bus.publish(event_2 { 3.0f }); // b doesn't implement event_2
	EXPECT_EQ(b.e1_calls, 0);

	bus.publish(event_1 { 7 });
	EXPECT_EQ(b.e1_calls, 1);
	EXPECT_EQ(b.e1_seen, (std::vector<int> { 7 }));
}

TEST(event_bus_jump_table, unsubscribe_stops_delivery) {
	event_bus<all_events> bus;
	listener_both a;

	auto h = bus.subscribe(a);

	bus.publish(event_1 { 1 });
	EXPECT_EQ(a.e1_calls, 1);

	bus.unsubscribe(h);

	bus.publish(event_1 { 2 });
	bus.publish(event_2 { 2.0f });

	EXPECT_EQ(a.e1_calls, 1);
	EXPECT_EQ(a.e2_calls, 0);
}

TEST(event_bus_jump_table, double_unsubscribe_is_safe) {
	event_bus<all_events> bus;
	listener_both a;

	auto h = bus.subscribe(a);

	bus.publish(event_1 { 1 });
	EXPECT_EQ(a.e1_calls, 1);

	bus.unsubscribe(h);
	bus.unsubscribe(h); // should be ignored

	bus.publish(event_1 { 2 });
	EXPECT_EQ(a.e1_calls, 1);
}

TEST(event_bus_jump_table, stale_handle_does_not_unsubscribe_new_listener_in_reused_slot) {
	event_bus<all_events> bus;

	listener_only_e1 first;
	listener_only_e1 second;

	// Register first, then remove it (frees a slot & increments generation)
	auto old_h = bus.subscribe(first);
	bus.unsubscribe(old_h);

	// Register second - likely reuses the freed slot
	auto new_h = bus.subscribe(second);

	// Using stale handle must NOT remove second
	bus.unsubscribe(old_h);

	bus.publish(event_1 { 42 });

	EXPECT_EQ(first.e1_calls, 0);
	EXPECT_EQ(second.e1_calls, 1);
	EXPECT_EQ(second.e1_seen, (std::vector<int> { 42 }));

	// sanity: new handle should unsubscribe
	bus.unsubscribe(new_h);
	bus.publish(event_1 { 100 });
	EXPECT_EQ(second.e1_calls, 1);
}

TEST(event_bus_jump_table, listener_with_no_handlers_is_ignored) {
	event_bus<all_events> bus;
	listener_none n;

	bus.subscribe(n);

	// Should not crash; no handlers exist
	bus.publish(event_1 { 1 });
	bus.publish(event_2 { 1.0f });

	SUCCEED();
}

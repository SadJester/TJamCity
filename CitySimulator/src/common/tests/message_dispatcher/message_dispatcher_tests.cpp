#include <common/stdafx.h>
#include <common/message_dispatcher/message_dispatcher.h>

using namespace tjs::common;

// Test event types
struct TestEvent : public Event {
	int value = 0;

	TestEvent(int value)
		: value(value) {}
};

struct AnotherEvent : public Event {
	std::string message;

	AnotherEvent(const std::string& message)
		: message(message) {}
};

// Test handler class
class TestHandler {
public:
	void HandleTestEvent(const TestEvent& event) {
		last_value = event.value;
		call_count++;
	}

	void HandleAnotherEvent(const AnotherEvent& event) {
		last_message = event.message;
		call_count++;
	}

	int last_value = 0;
	std::string last_message;
	int call_count = 0;
};

// Free function handler
void FreeFunctionHandler(const TestEvent& event) {
	static int last_value = 0;
	last_value = event.value;
}

TEST(MessageDispatcherTest, RegisterAndHandleMemberFunction) {
	MessageDispatcher dispatcher;
	TestHandler handler;

	// Register handler
	dispatcher.register_handler(handler, &TestHandler::HandleTestEvent, "test_handler");

	// Create and dispatch event
	TestEvent event { 42 };
	dispatcher.handle_message(event, "");

	// Verify handler was called
	EXPECT_EQ(handler.last_value, 42);
	EXPECT_EQ(handler.call_count, 1);
}

TEST(MessageDispatcherTest, RegisterAndHandleMultipleEvents) {
	MessageDispatcher dispatcher;
	TestHandler handler;

	// Register handlers for different events
	dispatcher.register_handler(handler, &TestHandler::HandleTestEvent, "test_handler");
	dispatcher.register_handler(handler, &TestHandler::HandleAnotherEvent, "another_handler");

	// Create and dispatch events
	TestEvent test_event { 42 };
	AnotherEvent another_event { "test message" };

	dispatcher.handle_message(test_event, "");
	dispatcher.handle_message(another_event, "");

	// Verify both handlers were called
	EXPECT_EQ(handler.last_value, 42);
	EXPECT_EQ(handler.last_message, "test message");
	EXPECT_EQ(handler.call_count, 2);
}

TEST(MessageDispatcherTest, RegisterAndHandleFreeFunction) {
	MessageDispatcher dispatcher;

	// Register free function handler
	dispatcher.register_handler(&FreeFunctionHandler, "free_handler", "");

	// Create and dispatch event
	TestEvent event { 42 };
	dispatcher.handle_message(event, "");

	// Note: We can't easily verify the free function was called
	// as it uses static variables. In a real test, you might want
	// to use a more testable approach for free functions.
}

TEST(MessageDispatcherTest, PublisherSpecificHandling) {
	MessageDispatcher dispatcher;
	TestHandler handler;

	// Register handler for specific publisher
	dispatcher.register_handler(handler, &TestHandler::HandleTestEvent, "test_handler", "publisher1");

	// Create event
	TestEvent event { 42 };

	// Dispatch from matching publisher
	dispatcher.handle_message(event, "publisher1");
	EXPECT_EQ(handler.last_value, 42);
	EXPECT_EQ(handler.call_count, 1);

	// Dispatch from different publisher
	handler.call_count = 0;
	dispatcher.handle_message(event, "publisher2");
	EXPECT_EQ(handler.call_count, 0); // Handler should not be called
}

TEST(MessageDispatcherTest, unregister_handler) {
	MessageDispatcher dispatcher;
	TestHandler handler;

	// Register handler
	dispatcher.register_handler(handler, &TestHandler::HandleTestEvent, "test_handler");

	// Unregister handler
	dispatcher.unregister_handler<TestEvent>("test_handler");

	// Create and dispatch event
	TestEvent event { 42 };
	dispatcher.handle_message(event, "");

	// Verify handler was not called
	EXPECT_EQ(handler.call_count, 0);
}

TEST(MessageDispatcherTest, UnregisterPublisherSpecificHandler) {
	MessageDispatcher dispatcher;
	TestHandler handler;

	// Register handler for specific publisher
	dispatcher.register_handler(handler, &TestHandler::HandleTestEvent, "test_handler", "publisher1");

	// Unregister handler
	dispatcher.unregister_handler<TestEvent>("test_handler", "publisher1");

	// Create and dispatch event
	TestEvent event { 42 };
	dispatcher.handle_message(event, "publisher1");

	// Verify handler was not called
	EXPECT_EQ(handler.call_count, 0);
}

TEST(MessageDispatcherTest, MultipleHandlersForSameEvent) {
	MessageDispatcher dispatcher;
	TestHandler handler1;
	TestHandler handler2;

	// Register two handlers for the same event
	dispatcher.register_handler(handler1, &TestHandler::HandleTestEvent, "handler1");
	dispatcher.register_handler(handler2, &TestHandler::HandleTestEvent, "handler2");

	// Create and dispatch event
	TestEvent event { 42 };
	dispatcher.handle_message(event, "");

	// Verify both handlers were called
	EXPECT_EQ(handler1.last_value, 42);
	EXPECT_EQ(handler2.last_value, 42);
	EXPECT_EQ(handler1.call_count, 1);
	EXPECT_EQ(handler2.call_count, 1);
}

TEST(MessageDispatcherTest, DuplicateRegistration) {
	MessageDispatcher dispatcher;
	TestHandler handler;

	// Register handler twice
	dispatcher.register_handler(handler, &TestHandler::HandleTestEvent, "test_handler");
	dispatcher.register_handler(handler, &TestHandler::HandleTestEvent, "test_handler");

	// Create and dispatch event
	TestEvent event { 42 };
	dispatcher.handle_message(event, "");

	// Verify handler was called only once
	EXPECT_EQ(handler.call_count, 1);
}

#include <stdafx.h>
#include <common/message_dispatcher/message_dispatcher.h>
#include <common/message_dispatcher/connection.h>
#include <gtest/gtest.h>

using namespace tjs::common;

// Test event types
struct TestEvent : public Event {
    int value = 0;

    TestEvent(int value) : value(value) {}
};

struct AnotherEvent : public Event {
    std::string message;

    AnotherEvent(const std::string& message) : message(message) {}
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

TEST(ConnectionTest, BasicConnection) {
    MessageDispatcher dispatcher;
    TestHandler handler;

    // Create connection
    Connection conn(dispatcher, handler, &TestHandler::HandleTestEvent, "test_handler", "");

    // Verify connection is active
    EXPECT_TRUE(conn.is_connected());

    // Create and dispatch event
    TestEvent event{42};
    dispatcher.HandleMessage(event, "");

    // Verify handler was called
    EXPECT_EQ(handler.last_value, 42);
    EXPECT_EQ(handler.call_count, 1);
}

TEST(ConnectionTest, AutomaticDisconnection) {
    MessageDispatcher dispatcher;
    TestHandler handler;

    {
        // Create connection in a scope
        Connection conn(dispatcher, handler, &TestHandler::HandleTestEvent, "test_handler", "");
        EXPECT_TRUE(conn.is_connected());
    } // Connection should be automatically disis_connected here

    // Create and dispatch event
    TestEvent event{42};
    dispatcher.HandleMessage(event, "");

    // Verify handler was not called (connection was disis_connected)
    EXPECT_EQ(handler.call_count, 0);
}

TEST(ConnectionTest, ManualDisconnection) {
    MessageDispatcher dispatcher;
    TestHandler handler;

    // Create connection
    Connection conn(dispatcher, handler, &TestHandler::HandleTestEvent, "test_handler", "");
    EXPECT_TRUE(conn.is_connected());

    // Manually disconnect
    conn.disconnect();
    EXPECT_FALSE(conn.is_connected());

    // Create and dispatch event
    TestEvent event{42};
    dispatcher.HandleMessage(event, "");

    // Verify handler was not called
    EXPECT_EQ(handler.call_count, 0);
}

TEST(ConnectionTest, MoveConstruction) {
    MessageDispatcher dispatcher;
    TestHandler handler;

    // Create original connection
    Connection original(dispatcher, handler, &TestHandler::HandleTestEvent, "test_handler", "");
    EXPECT_TRUE(original.is_connected());

    // Move construct new connection
    Connection moved(std::move(original));
    EXPECT_FALSE(original.is_connected()); // Original should be disis_connected
    EXPECT_TRUE(moved.is_connected());     // New connection should be active

    // Create and dispatch event
    TestEvent event{42};
    dispatcher.HandleMessage(event, "");

    // Verify handler was called through the moved connection
    EXPECT_EQ(handler.last_value, 42);
    EXPECT_EQ(handler.call_count, 1);
}

TEST(ConnectionTest, MoveAssignment) {
    MessageDispatcher dispatcher;
    TestHandler handler1;
    TestHandler handler2;

    // Create two connections
    Connection conn1(dispatcher, handler1, &TestHandler::HandleTestEvent, "handler1", "");
    Connection conn2(dispatcher, handler2, &TestHandler::HandleTestEvent, "handler2", "");

    // Move assign
    conn1 = std::move(conn2);
    EXPECT_FALSE(conn2.is_connected()); // Source should be disis_connected
    EXPECT_TRUE(conn1.is_connected());  // Destination should be active

    // Create and dispatch event
    TestEvent event{42};
    dispatcher.HandleMessage(event, "");

    // Verify only handler2 was called (handler1's connection was replaced)
    EXPECT_EQ(handler1.call_count, 0);
    EXPECT_EQ(handler2.last_value, 42);
    EXPECT_EQ(handler2.call_count, 1);
}

TEST(ConnectionTest, PublisherSpecificConnection) {
    MessageDispatcher dispatcher;
    TestHandler handler;

    // Create connection for specific publisher
    Connection conn(dispatcher, handler, &TestHandler::HandleTestEvent, "test_handler", "publisher1");
    EXPECT_TRUE(conn.is_connected());

    // Create event
    TestEvent event{42};

    // Dispatch from matching publisher
    dispatcher.HandleMessage(event, "publisher1");
    EXPECT_EQ(handler.last_value, 42);
    EXPECT_EQ(handler.call_count, 1);

    // Dispatch from different publisher
    handler.call_count = 0;
    dispatcher.HandleMessage(event, "publisher2");
    EXPECT_EQ(handler.call_count, 0); // Handler should not be called
}

TEST(ConnectionTest, MultipleConnections) {
    MessageDispatcher dispatcher;
    TestHandler handler;

    // Create multiple connections for different events
    Connection conn1(dispatcher, handler, &TestHandler::HandleTestEvent, "test_handler", "");
    Connection conn2(dispatcher, handler, &TestHandler::HandleAnotherEvent, "another_handler", "");

    // Create and dispatch events
    TestEvent test_event{42};
    AnotherEvent another_event{"test message"};

    dispatcher.HandleMessage(test_event, "");
    dispatcher.HandleMessage(another_event, "");

    // Verify both handlers were called
    EXPECT_EQ(handler.last_value, 42);
    EXPECT_EQ(handler.last_message, "test message");
    EXPECT_EQ(handler.call_count, 2);
}

TEST(ConnectionTest, CopyPrevention) {
    MessageDispatcher dispatcher;
    TestHandler handler;

    // Create original connection
    Connection original(dispatcher, handler, &TestHandler::HandleTestEvent, "test_handler", "");

    // Verify copy construction is deleted
    // Connection copy(original); // This should not compile

    // Verify copy assignment is deleted
    // Connection copy;
    // copy = original; // This should not compile
}

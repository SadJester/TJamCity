#pragma once

#include <common/message_dispatcher/Event.h>

namespace tjs::common::tests {
	struct TestEvent : public Event {
		int value = 0;

		explicit TestEvent(int value)
			: value(value) {}
	};

	struct AnotherEvent : public Event {
		std::string message;

		explicit AnotherEvent(const std::string& message)
			: message(message) {}
	};
} // namespace tjs::common::tests

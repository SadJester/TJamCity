#pragma once

#include <common/patterns/ilistener.h>

#include <visual_system/visual_system_definitions.h>

namespace tjs::ui {
	using ui_handling_events = common::type_list<
		visualization::visual_sys_lossy_queue::message_t,
		visualization::visual_sys_lossless_queue::message_t>;

	using ui_event_bus = common::event_bus<ui_handling_events>;

	template<typename T>
	using ui_listener = common::listener_crtp<T, ui_handling_events>;
} // namespace tjs::ui

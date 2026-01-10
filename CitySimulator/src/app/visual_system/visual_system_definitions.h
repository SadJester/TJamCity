#pragma once

#include <visual_system/visual_system_commands.h>
#include <visual_system/visual_system_events.h>

#include <common/sync/spmc_queue.h>
#include <common/sync/commands_queue.h>
#include <common/patterns/ilistener.h>

#include <events/project_events.h>

namespace tjs::visualization {
	class VisualSystem;

	using visual_system_commands = std::variant<UpdateRenderParamsCommand>;
	using visual_sys_lossy_queue = common::sync::spmc_queue<VisualSystemEvents>;
	// TODO: Correct impl of lossless
	using visual_sys_lossless_queue = common::sync::spmc_queue<VisualSystemEvents, 2048>;

	using visual_handling_events = common::type_list<
		events::OpenMapEvent>;

	using visual_event_bus = common::event_bus<visual_handling_events>;
} // namespace tjs::visualization

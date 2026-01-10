#pragma once

#include <visual_system/visual_system_commands.h>
#include <visual_system/visual_system_events.h>

#include <common/sync/spmc_queue.h>
#include <common/sync/commands_queue.h>
#include <common/patterns/ilistener.h>

namespace tjs::visualization {
	class VisualSystem;

	using visual_system_commands = std::variant<UpdateRenderParamsCommand>;
	using visual_sys_lossy_queue = common::sync::spmc_queue<VisualSystemEvents>;
	// TODO: Correct impl of lossless
	using visual_sys_lossless_queue = common::sync::spmc_queue<VisualSystemEvents, 2048>;
} // namespace tjs::visualization

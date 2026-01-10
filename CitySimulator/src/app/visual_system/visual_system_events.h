#pragma once

#include <visual_system/visual_system_commands.h>

namespace tjs::core {
	struct AgentData;
	struct Lane;
	struct VehicleData;
} // namespace tjs::core

namespace tjs::visualization {
	enum class VisualSystemEvents {
		agent_selected,
		lane_selected,
		map_position_changed
	};

	struct agent_payload {
		static constexpr VisualSystemEvents ALLOWED_IN_MESSAGES[] = { VisualSystemEvents::agent_selected };
		core::AgentData* agent;

		agent_payload(core::AgentData* agent)
			: agent(agent) {
		}
	};

	struct lane_payload {
		static constexpr VisualSystemEvents ALLOWED_IN_MESSAGES[] = { VisualSystemEvents::lane_selected };
		core::Lane* lane;

		lane_payload(core::Lane* lane)
			: lane(lane) {
		}
	};

} // namespace tjs::visualization

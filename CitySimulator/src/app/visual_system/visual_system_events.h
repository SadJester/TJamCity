#pragma once

#include <visual_system/visual_system_commands.h>

namespace tjs::core {
	struct AgentData;
} // namespace tjs::core

namespace tjs::visualization {
	enum class VisualSystemEvents {
		agent_selected
	};

	struct agent_payload {
		static constexpr VisualSystemEvents ALLOWED_IN_MESSAGES[] = { VisualSystemEvents::agent_selected };
		core::AgentData* agent;

		agent_payload(core::AgentData* agent)
			: agent(agent) {
		}
	};

} // namespace tjs::visualization

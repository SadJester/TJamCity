#pragma once

#include <common/message_dispatcher/Event.h>
#include <core/simulation/agent/agent_data.h>

namespace tjs::events {
	struct AgentSelected : common::Event {
		core::AgentData* agent;

		AgentSelected(core::AgentData* agent)
			: agent(agent) {}
	};
} // namespace tjs::events

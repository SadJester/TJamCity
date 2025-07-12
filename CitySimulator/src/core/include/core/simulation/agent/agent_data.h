#pragma once

#include "core/data_layer/data_types.h"

namespace tjs::core {
	struct Node;
} // namespace tjs::core

namespace tjs::core {
	ENUM_FLAG(TacticalBehaviour, char,
		Normal,
		Aggressive,
		Defensive,
		Emergency);

	struct AgentData {
		uint64_t id;
		TacticalBehaviour behaviour;
		core::Node* currentGoal;
		tjs::core::Vehicle* vehicle;
		std::vector<core::Edge*> path; // Path to follow
		double distanceTraveled = 0.0; // Total distance traveled
		bool stucked = false;
		int goalFailCount = 0;
	};
	//static_assert(std::is_pod<AgentData>::value, "AgentData must be POD");
} // namespace tjs::core

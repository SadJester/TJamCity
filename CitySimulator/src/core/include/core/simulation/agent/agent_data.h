#pragma once

#include "core/data_layer/data_types.h"

namespace tjs::core {
	struct Node;
} // namespace tjs::core

namespace tjs::core {
	ENUM_FLAG(TacticalBehaviour,
		Normal,
		Aggressive,
		Defensive,
		Emergency);

	struct AgentData {
		uint64_t id;
		TacticalBehaviour behaviour;
		core::Node* currentGoal;
		core::Coordinates currentStepGoal;
		tjs::core::Vehicle* vehicle;
		std::deque<core::Node*> path; // Path to follow
		std::vector<core::Node*> visitedNodes;
		bool last_segment = false;
		double distanceTraveled = 0.0; // Total distance traveled
	};
	//static_assert(std::is_pod<AgentData>::value, "AgentData must be POD");
} // namespace tjs::core

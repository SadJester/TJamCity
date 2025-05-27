#pragma once

#include "core/data_layer/data_types.h"

namespace tjs::simulation {
	ENUM_FLAG(TacticalBehaviour,
		Normal,
		Aggressive,
		Defensive,
		Emergency);

	struct AgentData {
		uint64_t id;
		TacticalBehaviour behaviour;
		core::Coordinates currentGoal;
		core::Coordinates currentStepGoal;
		tjs::core::Vehicle* vehicle;
	};
	static_assert(std::is_pod<AgentData>::value, "AgentData must be POD");
} // namespace tjs::simulation

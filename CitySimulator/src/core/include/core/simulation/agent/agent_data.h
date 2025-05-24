#pragma once

#include "core/dataLayer/data_types.h"

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
		tjs::core::Vehicle* vehicle;
	};
	static_assert(std::is_pod<AgentData>::value, "AgentData must be POD");
} // namespace tjs::simulation

#pragma once

#include <core/data_layer/data_types.h>

#include <core/simulation/simulation_types.h>

namespace tjs::core {
	struct Node;
} // namespace tjs::core

namespace tjs::core {

	struct AgentProfile {
		AgentGoalSelectionType goal_selection = AgentGoalSelectionType::RandomSelection;
		// profile -> school, work, shopping
	};

	struct AgentData {
		uint64_t id;
		TacticalBehaviour behaviour;
		AgentProfile profile;
		core::Node* currentGoal = nullptr;
		tjs::core::Vehicle* vehicle = nullptr;
		std::vector<core::Edge*> path; // Path to follow
		size_t path_offset;
		uint32_t goal_lane_mask = 0;   // bitmask for current edge exit
		double distanceTraveled = 0.0; // Total distance traveled
		bool stucked = false;
		int goalFailCount = 0;

		AgentData(uint64_t uid, Vehicle* vehicle_ = nullptr)
			: id(uid)
			, vehicle(vehicle_) {
		}
	};
	//static_assert(std::is_pod<AgentData>::value, "AgentData must be POD");
} // namespace tjs::core

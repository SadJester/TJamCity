#pragma once

#include <core/data_layer/data_types.h>

#include <core/simulation/simulation_types.h>

namespace tjs::core {
	struct Node;
	struct Vehicle;
} // namespace tjs::core

namespace tjs::core {

	struct AgentProfile {
		AgentGoalSelectionType goal_selection = AgentGoalSelectionType::RandomSelection;
		Node* goal = nullptr;
		// profile -> school, work, shopping
	};

	struct AgentData {
		uint64_t id;
		TacticalBehaviour behaviour = TacticalBehaviour::Normal;
		AgentProfile profile;
		core::Node* currentGoal = nullptr;
		Vehicle* vehicle = nullptr;
		std::vector<core::Edge*> path; // Path to follow
		size_t path_offset = 0;
		uint32_t goal_lane_mask = 0;   // bitmask for current edge exit
		double distanceTraveled = 0.0; // Total distance traveled
		bool stucked = false;
		bool to_remove = false;
		int goalFailCount = 0;

		AgentData(uint64_t uid, Vehicle* vehicle_ = nullptr)
			: id(uid)
			, vehicle(vehicle_) {
		}
	};
	//static_assert(std::is_pod<AgentData>::value, "AgentData must be POD");
} // namespace tjs::core

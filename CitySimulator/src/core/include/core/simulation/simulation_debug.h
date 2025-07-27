#pragma once

#include <core/store_models/idata_model.h>
#include <nlohmann/json.hpp>

namespace tjs::core {
	struct Node;
} // namespace tjs::core

namespace tjs::core::simulation {
	ENUM(SimulationMovementPhase, char,
		None,
		IDM_Phase1_Lane,
		IDM_Phase1_Vehicle,
		IDM_Phase2_Agent,
		IDM_Phase2_ChooseLane);

	struct SimulationDebugData : public model::IDataModel {
		static std::type_index get_type() { return typeid(SimulationDebugData); }

		Node* selectedNode = nullptr;
		std::unordered_set<uint64_t> reachableNodes;

		std::vector<size_t> vehicle_indices;    // indices of vehicles that should be in the lane
		size_t lane_id;                         // Lane id to break
		size_t agent_id;                        // agent id that should be break
		SimulationMovementPhase movement_phase; // at what phase should break

		void reinit() override {
			selectedNode = nullptr;
			reachableNodes.clear();
		}

		void assign(const SimulationDebugData& other) {
			selectedNode = other.selectedNode;
			lane_id = other.lane_id;
			agent_id = other.agent_id;
			movement_phase = other.movement_phase;
			vehicle_indices = other.vehicle_indices;
		}

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimulationDebugData,
			lane_id,
			agent_id,
			vehicle_indices,
			movement_phase)
	};
} // namespace tjs::core::simulation

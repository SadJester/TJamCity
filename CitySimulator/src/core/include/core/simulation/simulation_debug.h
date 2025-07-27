#pragma once

#include <core/store_models/idata_model.h>

namespace tjs::core {
	struct Node;
} // namespace tjs::core

namespace tjs::core::simulation {
	ENUM(SimulationMovementPhase, char,
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
		SimulationMovementPhase movement_phase; // at what phase should break

		void reinit() override {
			selectedNode = nullptr;
			vehicle_indices.clear();
			lane_id = 0;
			movement_phase = SimulationMovementPhase::IDM_Phase1_Lane;
		}
	};
} // namespace tjs::core::simulation

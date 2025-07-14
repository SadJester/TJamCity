#pragma once

#include <core/store_models/idata_model.h>

namespace tjs::core {
	struct Node;
} // namespace tjs::core

namespace tjs::core::model {

	struct SimulationDebugData : public IDataModel {
		static std::type_index get_type() { return typeid(SimulationDebugData); }

		Node* selectedNode = nullptr;
		std::unordered_set<uint64_t> reachableNodes;

		void reinit() override {
			selectedNode = nullptr;
		}
	};

} // namespace tjs::core::model

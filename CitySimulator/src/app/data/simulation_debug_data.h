#pragma once

#include <core/store_models/idata_model.h>
#include <visualization/persistent_data/map_elements_data.h>

namespace tjs::core::model {

	struct SimulationDebugData : public IDataModel {
		static std::type_index get_type() { return typeid(SimulationDebugData); }

		visualization::NodeRenderInfo* selectedNode = nullptr;
		std::unordered_set<uint64_t> reachableNodes;

		void reinit() override {
			selectedNode = nullptr;
		}
	};

} // namespace tjs::core::model

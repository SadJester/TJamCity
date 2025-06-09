#pragma once

#include <core/store_models/idata_model.h>
#include <visualization/persistent_data/map_elements_data.h>


namespace tjs::core::model {

	struct PersistentRenderData : public IDataModel {
		static std::type_index get_type() { return typeid(PersistentRenderData); }

		std::unordered_map<uint64_t, visualization::NodeRenderInfo> nodes;
                std::unordered_map<uint64_t, visualization::WayRenderInfo> ways;
                std::vector<visualization::VehicleRenderInfo> vehicles;

                visualization::NodeRenderInfo* selectedNode = nullptr;
                std::unordered_set<uint64_t> reachableNodes;
        };

} // namespace tjs::core::model

namespace tjs {
	class Application;
} // namespace tjs

namespace tjs::visualization {
	void recalculate_map_data(Application& app);
} // namespace tjs::visualization

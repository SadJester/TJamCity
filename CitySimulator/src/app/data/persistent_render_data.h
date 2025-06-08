#pragma once

#include <core/store_models/idata_model.h>
#include <core/data_layer/data_types.h>
#include <render/render_primitives.h>
#include <unordered_map>
#include <vector>

namespace tjs::core::model {

	struct NodeRenderInfo {
		core::Node* node = nullptr;
		tjs::Position screenPos {};
		bool selected = false;
	};

	struct WayRenderInfo {
		core::WayInfo* way = nullptr;
		std::vector<tjs::Position> screenPoints;
		bool selected = false;
	};

	struct VehicleRenderInfo {
		core::Vehicle* vehicle = nullptr;
		tjs::Position screenPos {};
	};

	struct PersistentRenderData : public IDataModel {
		static std::type_index get_type() { return typeid(PersistentRenderData); }

		std::unordered_map<uint64_t, NodeRenderInfo> nodes;
		std::unordered_map<uint64_t, WayRenderInfo> ways;
		std::vector<VehicleRenderInfo> vehicles;

		NodeRenderInfo* selectedNode = nullptr;
	};

} // namespace tjs::core::model

namespace tjs {
	class Application;
} // namespace tjs

namespace tjs::visualization {
	void recalculate_map_data(Application& app);
} // namespace tjs::visualization

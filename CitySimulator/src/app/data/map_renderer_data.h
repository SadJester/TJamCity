#pragma once

#include <core/store_models/idata_model.h>
#include <render/render_primitives.h>

namespace tjs::core {
	struct Lane;
} // namespace tjs::core

namespace tjs::core::model {

	ENUM_FLAG(MapRendererLayer, char,
		None = 0,
		Ways = 1 << 0,
		Nodes = 1 << 1,
		TrafficLights = 1 << 2,
		NetworkGraph = 1 << 3,
		All = Ways | Nodes | TrafficLights | NetworkGraph);

	struct MapRendererData : public IDataModel {
		static std::type_index get_type() {
			return typeid(MapRendererData);
		}

		// View settings
		double metersPerPixel = 1.0;

		// Layer visibility flags
		MapRendererLayer visibleLayers = MapRendererLayer::Ways;

		// Rendering options
		bool showBoundingBox = false;
		bool showLaneMarkers = true;
		double laneMarkerVisibilityThreshold = 1.0; // meters per pixel threshold
		double simplifiedViewThreshold = 1.2;       // meters per pixel threshold

		bool networkOnlyForSelected = false;

		core::Lane* selected_lane = nullptr;

		Position screen_center;

		MapRendererData() = default;

		void set_meters_per_pixel(double metersPerPixel);

		void reinit() override {
			selected_lane = nullptr;
		}
	};

} // namespace tjs::core::model

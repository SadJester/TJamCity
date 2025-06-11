#pragma once

#include <core/store_models/idata_model.h>
#include <core/data_layer/data_types.h>
#include <render/render_primitives.h>

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
		core::Coordinates projectionCenter;
		double metersPerPixel = 1.0;

		// Layer visibility flags
		MapRendererLayer visibleLayers = MapRendererLayer::Ways;

		// Rendering options
		bool showBoundingBox = false;
		bool showLaneMarkers = true;
		double laneMarkerVisibilityThreshold = 50.0; // meters per pixel threshold

		bool networkOnlyForSelected = false;

		Position screen_center;

		MapRendererData() = default;

		void set_meters_per_pixel(double metersPerPixel);
	};

} // namespace tjs::core::model

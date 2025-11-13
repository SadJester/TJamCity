#include <stdafx.h>

#include <data/map_renderer_data.h>

namespace tjs::core::model {

	void MapRendererData::set_meters_per_pixel(double metersPerPixel) {
		this->metersPerPixel = metersPerPixel;
	}

	void MapRendererData::sync(MapRendererData& dst, const MapRendererData& src) {
		dst.metersPerPixel = src.metersPerPixel;
		dst.showBoundingBox = src.showBoundingBox;
		dst.laneMarkerVisibilityThreshold = src.laneMarkerVisibilityThreshold;
		dst.simplifiedViewThreshold = src.simplifiedViewThreshold;

		dst.networkOnlyForSelected = src.networkOnlyForSelected;
		dst.selected_lane = src.selected_lane;
		dst.screen_center = src.screen_center;
	}

} // namespace tjs::core::model

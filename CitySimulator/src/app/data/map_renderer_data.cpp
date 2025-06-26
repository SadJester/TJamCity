#include <stdafx.h>

#include <data/map_renderer_data.h>

namespace tjs::core::model {

	void MapRendererData::set_meters_per_pixel(double metersPerPixel) {
		this->metersPerPixel = metersPerPixel;
	}

} // namespace tjs::core::model

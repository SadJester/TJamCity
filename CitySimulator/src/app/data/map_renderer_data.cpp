#include <stdafx.h>

#include <data/map_renderer_data.h>

#include <core/math_constants.h>

namespace tjs::core::model {

	void MapRendererData::set_meters_per_pixel(double metersPerPixel) {
		this->metersPerPixel = metersPerPixel;
		double latRad = projectionCenter.latitude * MathConstants::DEG_TO_RAD;
		//this->metersPerPixel *= std::cos(latRad);
	}

} // namespace tjs::core::model

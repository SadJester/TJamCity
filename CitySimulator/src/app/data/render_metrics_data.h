#pragma once

#include <core/store_models/idata_model.h>

namespace tjs::core::model {
	struct RenderMetricsData : public IDataModel {
		static std::type_index get_type() { return typeid(RenderMetricsData); }

		std::size_t triangles_last_frame = 0;

		void reinit() override { triangles_last_frame = 0; }
	};
} // namespace tjs::core::model

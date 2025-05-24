#pragma once

#include <nlohmann/json.hpp>

#include <core/dataLayer/data_types.h>

namespace tjs::settings {

	struct GeneralSettings {
		std::string selectedFile;
		core::Coordinates projectionCenter;
		double zoomLevel;

		static constexpr const char* NAME = "General";
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(GeneralSettings,
			selectedFile,
			projectionCenter,
			zoomLevel)
	};
} // namespace tjs::settings

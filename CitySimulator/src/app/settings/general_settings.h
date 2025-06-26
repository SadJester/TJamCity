#pragma once

#include <nlohmann/json.hpp>

#include <render/render_primitives.h>

namespace tjs::settings {

	struct GeneralSettings {
		std::string selectedFile;
		tjs::Position screen_center;
		double zoomLevel;

		static constexpr const char* NAME = "General";
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(GeneralSettings,
			selectedFile,
			screen_center,
			zoomLevel)
	};
} // namespace tjs::settings

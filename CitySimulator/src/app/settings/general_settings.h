#pragma once

#include <nlohmann/json.hpp>

#include <render/render_primitives.h>

namespace tjs::settings {

	struct GeneralSettings {
		std::string selectedFile;
		tjs::Position screen_center;
		double zoomLevel;
		tjs::Rectangle qt_window { 0, 0, 700, 800 };
		tjs::Rectangle sdl_window { 0, 0, 1024, 768 };

		static constexpr const char* NAME = "General";
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(GeneralSettings,
			selectedFile,
			screen_center,
			zoomLevel,
			qt_window,
			sdl_window)
	};
} // namespace tjs::settings

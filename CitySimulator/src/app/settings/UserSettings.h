#pragma once

#include "settings/UserSettingsLoader.h"

#include "settings/GeneralSettings.h"
#include "settings/RenderSettings.h"
#include <core/simulation/simulation_settings.h>

namespace tjs {

	class UserSettings {
	public:
		static constexpr const char* APP_NAME = "TJamSimulator";
		static constexpr const char* SETTINGS_FILE = "settings.json";
		static constexpr const float AUTOSAVE_TIME_SEC = 10;

		settings::GeneralSettings general;
		settings::RenderSettings render;
		core::SimulationSettings simulationSettings;

		void load() {
			settings::UserSettingsLoader<> parser;
			parser.load(APP_NAME, SETTINGS_FILE);
			general = parser.get<settings::GeneralSettings>(settings::GeneralSettings::NAME);
			render = parser.get<settings::RenderSettings>(settings::RenderSettings::NAME);
			simulationSettings = parser.get<core::SimulationSettings>(core::SimulationSettings::NAME);
		}

		void save() {
			settings::UserSettingsLoader<> parser;
			parser.setConfigPath(fs::FileLocator::getConfigPath(APP_NAME, SETTINGS_FILE));

			parser.set(settings::GeneralSettings::NAME, general);
			parser.set(settings::RenderSettings::NAME, render);
			parser.set(core::SimulationSettings::NAME, simulationSettings);

			parser.save();
		}
	};

} // namespace tjs

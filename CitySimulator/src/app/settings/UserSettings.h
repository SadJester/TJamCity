#pragma once

#include "settings/UserSettingsLoader.h"

#include "settings/GeneralSettings.h"
#include "settings/RenderSettings.h"

namespace tjs {

    class UserSettings {
    public:
        static constexpr const char* APP_NAME = "TJamSimulator";

        settings::GeneralSettings general;
        settings::RenderSettings render;

        void load() {
            settings::UserSettingsLoader<> parser;
            parser.load(APP_NAME);
            general = parser.get<settings::GeneralSettings>(settings::GeneralSettings::NAME);
            render = parser.get<settings::RenderSettings>(settings::RenderSettings::NAME);
        }

        void save() {
            settings::UserSettingsLoader<> parser;
            parser.setConfigPath(fs::FileLocator::getConfigPath(APP_NAME));
            
            parser.set(settings::GeneralSettings::NAME, general);
            parser.set(settings::RenderSettings::NAME, render);

            parser.save();
        }
    };

}

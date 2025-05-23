#pragma once

#include <core/simulation/simulation_settings.h>

namespace tjs::core
{
        class WorldData;

        class WorldCreator final {
        public:
            static bool loadOSMData(WorldData& data, std::string_view osmFilename);
            static bool createRandomVehicles(WorldData& data, const SimulationSettings& settings);

        private:
            WorldCreator() = delete;

            static bool loadOSMXmlData(WorldData& data, std::string_view osmFileName);
        };
} // namespace tjs

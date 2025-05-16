#pragma once

namespace tjs::core
{
        class WorldData;

        class WorldCreator final {
        public:
            static bool loadOSMData(WorldData& data, std::string_view osmFilename);
            static bool createVehicles(WorldData& data, size_t vehiclesCount);

        private:
            WorldCreator() = delete;

            static bool loadOSMXmlData(WorldData& data, std::string_view osmFileName);
        };
} // namespace tjs

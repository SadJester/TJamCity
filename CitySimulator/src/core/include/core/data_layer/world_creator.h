#pragma once

#include <core/simulation/simulation_settings.h>

namespace tjs::core {
	class WorldData;
	struct WorldSegment;

	namespace details {
		bool loadOSMXmlData(WorldData& data, std::string_view osmFileName);
		void preprocess_segment(WorldSegment& segment);
		void create_road_network(WorldSegment& segment);
	}

	class WorldCreator final {
	public:
		static bool loadOSMData(WorldData& data, std::string_view osmFilename);
		static bool createRandomVehicles(WorldData& data, const SimulationSettings& settings);
	private:
		WorldCreator() = delete;
	};
} // namespace tjs::core

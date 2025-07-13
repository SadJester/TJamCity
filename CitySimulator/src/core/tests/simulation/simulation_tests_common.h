#pragma once

#include <core/data_layer/world_data.h>
#include <core/store_models/idata_model.h>
#include <core/simulation/simulation_system.h>

namespace tjs::core::tests {
	class SimulationTestsCommon {
	public:
		void create_basic_system();

	protected:
		tjs::core::WorldData world;
		tjs::core::model::DataModelStore store;
		std::unique_ptr<tjs::core::simulation::TrafficSimulationSystem> system;
		tjs::core::SimulationSettings settings;
	};
} // namespace tjs::core::tests

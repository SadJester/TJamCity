#include <stdafx.h>

#include <simulation/simulation_tests_common.h>

#include <core/random_generator.h>
#include <core/store_models/vehicle_analyze_data.h>
#include <core/simulation/simulation_system.h>
#include <core/data_layer/world_creator.h>

#include <data_layer/world_utils.h>
#include <core/map_math/contraction_builder.h>
#include <core/map_math/lane_connector_builder.h>

#include <core/simulation/transport_management/vehicle_state.h>

namespace tjs::core::tests {

	void SimulationTestsCommon::SetUp() {
		Lane::reset_id();
		Edge::reset_id();

		ASSERT_TRUE(load_map());
		ASSERT_TRUE(prepare());
		set_up_settings();
	}

	bool SimulationTestsCommon::prepare() {
		core::details::preprocess_segment(*world.segments()[0]);

		algo::ContractionBuilder builder;
		builder.build_graph(*world.segments()[0]->road_network);

		return true;
	}

	void SimulationTestsCommon::set_up_settings() {
		settings.vehiclesCount = 1;
		settings.movement_algo = simulation::MovementAlgoType::Agent;
		settings.randomSeed = false;
		settings.seedValue = 42;
	}

	bool SimulationTestsCommon::load_map() {
		bool result = core::WorldCreator::loadOSMData(world, data_file(default_map()).string());
		return world.segments().size() == 1;
	}

	void SimulationTestsCommon::create_basic_system() {
		using namespace tjs::core::simulation;

		set_up_settings();
		store.create<model::VehicleAnalyzeData>();

		system = std::make_unique<tjs::core::simulation::TrafficSimulationSystem>(world, store, settings);
		system->initialize();

		system->step();
	}
} // namespace tjs::core::tests

#include <stdafx.h>

#include <simulation/simulation_tests_common.h>

#include <core/random_generator.h>
#include <core/store_models/vehicle_analyze_data.h>
#include <core/simulation/simulation_system.h>
#include <core/data_layer/world_creator.h>

#include <data_layer/world_utils.h>
#include <core/map_math/contraction_builder.h>
#include <core/map_math/lane_connector_builder.h>

namespace tjs::core::tests {

	void SimulationTestsCommon::SetUp() {
		Lane::reset_id();
		Edge::reset_id();
		ASSERT_TRUE(load_map());
		ASSERT_TRUE(prepare());
	}

	bool SimulationTestsCommon::prepare() {
		core::details::preprocess_segment(*world.segments()[0]);

		algo::ContractionBuilder builder;
		builder.build_graph(*world.segments()[0]->road_network);

		return true;
	}

	bool SimulationTestsCommon::load_map() {
		bool result = core::WorldCreator::loadOSMData(world, data_file(default_map()).string());
		return world.segments().size() == 1;
	}

	void SimulationTestsCommon::create_basic_system() {
		Vehicle v {};
		v.uid = 1;
		v.type = VehicleType::SimpleCar;
		v.currentSpeed = 0.0f;
		v.maxSpeed = 60.0f;
		v.coordinates = world.segments().front()->nodes.begin()->second->coordinates;
		v.currentWay = nullptr;
		v.currentSegmentIndex = 0;
		v.current_lane = nullptr;
		v.s_on_lane = 0.0;
		v.lateral_offset = 0.0;
		v.state_ = 0;         //
		v.previous_state = 0; //VehicleState::Stopped;

		system->vehicle_system().vehicles().push_back(v);

		store.create<model::VehicleAnalyzeData>();
		system = std::make_unique<tjs::core::simulation::TrafficSimulationSystem>(world, store, settings);
		system->initialize();
		RandomGenerator::set_seed(42);
	}
} // namespace tjs::core::tests

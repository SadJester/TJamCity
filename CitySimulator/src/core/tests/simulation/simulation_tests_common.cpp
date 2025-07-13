#include <stdafx.h>

#include <simulation/simulation_tests_common.h>

#include <core/random_generator.h>
#include <core/store_models/vehicle_analyze_data.h>
#include <core/simulation/simulation_system.h>

namespace tjs::core::tests {
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
		world.vehicles().push_back(v);

		store.create<model::VehicleAnalyzeData>();
		system = std::make_unique<tjs::core::simulation::TrafficSimulationSystem>(world, store, settings);
		system->initialize();
		RandomGenerator::set_seed(42);
	}
} // namespace tjs::core::tests

#include "stdafx.h"

#include <core/data_layer/world_creator.h>
#include <core/data_layer/world_data.h>
#include <core/simulation/simulation_system.h>
#include <core/store_models/vehicle_analyze_data.h>
#include <core/random_generator.h>

#include <filesystem>

using namespace tjs::core;
using namespace tjs::core::simulation;

namespace {
	std::filesystem::path data_file(const char* name) {
		return std::filesystem::path(__FILE__).parent_path() / "test_data" / name;
	}
} // namespace

class SimulationModuleTest : public ::testing::Test {
protected:
	WorldData world;
	model::DataModelStore store;
	std::unique_ptr<TrafficSimulationSystem> system;

	void SetUp() override {
		ASSERT_TRUE(WorldCreator::loadOSMData(world, data_file("simple_grid.osmx").string()));

		Vehicle v {};
		v.uid = 1;
		v.type = VehicleType::SimpleCar;
		v.currentSpeed = 0.0f;
		v.maxSpeed = 60.0f;
		v.coordinates = world.segments().front()->nodes.begin()->second->coordinates;
		v.currentWay = nullptr;
		v.currentSegmentIndex = 0;
		world.vehicles().push_back(v);

		store.add_model<model::VehicleAnalyzeData>();
		system = std::make_unique<TrafficSimulationSystem>(world, store);
		system->initialize();
		RandomGenerator::set_seed(42);
	}
};

TEST_F(SimulationModuleTest, StrategicSetsGoal) {
	auto& agent = system->agents()[0];
	EXPECT_EQ(agent.currentGoal, nullptr);
	system->strategicModule().update();
	EXPECT_NE(agent.currentGoal, nullptr);
}

TEST_F(SimulationModuleTest, TacticalBuildsPath) {
	auto& agent = system->agents()[0];
	system->strategicModule().update();
	system->tacticalModule().update();
	EXPECT_GT(agent.path.size(), 0u);
}

TEST_F(SimulationModuleTest, VehicleMovesTowardsGoal) {
	auto& agent = system->agents()[0];
	system->strategicModule().update();
	system->tacticalModule().update();
	Coordinates start = agent.vehicle->coordinates;
	system->timeModule().update(0.016);
	system->vehicleMovementModule().update();
	EXPECT_NE(start.latitude, agent.vehicle->coordinates.latitude);
	EXPECT_NE(start.longitude, agent.vehicle->coordinates.longitude);
}

TEST_F(SimulationModuleTest, TacticalMarksAgentStuckAfterFailures) {
	auto& agent = system->agents()[0];
	auto unreachable = tjs::core::Node::create(9999, agent.vehicle->coordinates, tjs::core::NodeTags::None);

	for (int i = 0; i < 5; ++i) {
		agent.currentGoal = unreachable.get();
		agent.path.clear();
		agent.last_segment = false;
		system->tacticalModule().update();
	}

	EXPECT_TRUE(agent.stucked);
	EXPECT_GE(agent.goalFailCount, 5);
}

TEST_F(SimulationModuleTest, StrategicIgnoresStuckAgents) {
	auto& agent = system->agents()[0];
	agent.stucked = true;
	agent.currentGoal = nullptr;
	system->strategicModule().update();
	EXPECT_EQ(agent.currentGoal, nullptr);
}

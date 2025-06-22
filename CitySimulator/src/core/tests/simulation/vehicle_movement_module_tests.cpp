#include "stdafx.h"

#include <data_loader_mixin.h>

#include <core/data_layer/world_creator.h>
#include <core/data_layer/world_data.h>
#include <core/simulation/simulation_system.h>
#include <core/simulation/movement/vehicle_movement_module.h>
#include <core/store_models/vehicle_analyze_data.h>
#include <core/random_generator.h>
#include <core/data_layer/road_network.h>
#include <core/math_constants.h>
#include <core/map_math/earth_math.h>

#include <filesystem>

using namespace tjs::core;
using namespace tjs::core::simulation;

class VehicleMovementModuleTest : public ::testing::Test, public tjs::core::tests::DataLoaderMixin {
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
		v.current_lane = nullptr;
		v.s_on_lane = 0.0;
		v.lateral_offset = 0.0;
		world.vehicles().push_back(v);

		store.add_model<model::VehicleAnalyzeData>();
		system = std::make_unique<TrafficSimulationSystem>(world, store);
		system->initialize();
		RandomGenerator::set_seed(42);
	}

	// Helper method to create a simple lane for testing
	Lane createTestLane(const Coordinates& start, const Coordinates& end) {
		Lane lane;
		lane.centerLine.push_back(start);
		lane.centerLine.push_back(end);
		return lane;
	}

	// Helper method to get initial agent state
	AgentData& getAgent() {
		return system->agents()[0];
	}
};

TEST_F(VehicleMovementModuleTest, NoMovementWhenCurrentGoalIsNullptr) {
	auto& agent = getAgent();

	// Ensure agent has no current goal
	agent.currentGoal = nullptr;

	// Set up vehicle with a valid lane
	auto testLane = createTestLane(
		Coordinates { 0.0, 0.0 },
		Coordinates { 0.001, 0.001 });
	agent.vehicle->current_lane = &testLane;

	// Record initial position
	Coordinates initialPosition = agent.vehicle->coordinates;
	double initialSOnLane = agent.vehicle->s_on_lane;

	// Update time and run movement
	system->timeModule().update(0.016); // 16ms delta time
	system->vehicleMovementModule().update();

	// Verify no movement occurred
	EXPECT_EQ(agent.vehicle->coordinates.x, initialPosition.x);
	EXPECT_EQ(agent.vehicle->coordinates.y, initialPosition.y);
	EXPECT_EQ(agent.vehicle->s_on_lane, initialSOnLane);
}

TEST_F(VehicleMovementModuleTest, NoMovementWhencurrent_laneIsNullptr) {
	auto& agent = getAgent();

	// Set up agent with a goal but no lane
	agent.currentGoal = world.segments().front()->nodes.begin()->second.get();
	agent.vehicle->current_lane = nullptr;

	// Record initial position
	Coordinates initialPosition = agent.vehicle->coordinates;
	double initialSOnLane = agent.vehicle->s_on_lane;

	// Update time and run movement
	system->timeModule().update(0.016);
	system->vehicleMovementModule().update();

	// Verify no movement occurred
	EXPECT_EQ(agent.vehicle->coordinates.x, initialPosition.x);
	EXPECT_EQ(agent.vehicle->coordinates.y, initialPosition.y);
	EXPECT_EQ(agent.vehicle->s_on_lane, initialSOnLane);
}

TEST_F(VehicleMovementModuleTest, NoMovementWhenBothCurrentGoalAndLaneAreNullptr) {
	auto& agent = getAgent();

	// Set both to nullptr
	agent.currentGoal = nullptr;
	agent.vehicle->current_lane = nullptr;

	// Record initial position
	Coordinates initialPosition = agent.vehicle->coordinates;
	double initialSOnLane = agent.vehicle->s_on_lane;

	// Update time and run movement
	system->timeModule().update(0.016);
	system->vehicleMovementModule().update();

	// Verify no movement occurred
	EXPECT_EQ(agent.vehicle->coordinates.x, initialPosition.x);
	EXPECT_EQ(agent.vehicle->coordinates.y, initialPosition.y);
	EXPECT_EQ(agent.vehicle->s_on_lane, initialSOnLane);
}

TEST_F(VehicleMovementModuleTest, MovementOccursWithValidGoalAndLane) {
	auto& agent = getAgent();

	// Set up agent with a goal
	agent.currentGoal = world.segments().front()->nodes.begin()->second.get();

	// Create a test lane
	auto testLane = createTestLane(
		Coordinates { 0.0, 0.0 },
		Coordinates { 0.001, 0.001 });
	agent.vehicle->current_lane = &testLane;

	// Record initial position
	Coordinates initialPosition = agent.vehicle->coordinates;
	double initialSOnLane = agent.vehicle->s_on_lane;

	// Update time and run movement
	system->timeModule().update(0.016);
	system->vehicleMovementModule().update();

	// Verify movement occurred
	EXPECT_NE(agent.vehicle->coordinates.x, initialPosition.x);
	EXPECT_NE(agent.vehicle->coordinates.y, initialPosition.y);
	EXPECT_GT(agent.vehicle->s_on_lane, initialSOnLane);
}

TEST_F(VehicleMovementModuleTest, SpeedIsSetToDefaultWhenMoving) {
	auto& agent = getAgent();

	// Set up agent with a goal
	agent.currentGoal = world.segments().front()->nodes.begin()->second.get();

	// Create a test lane
	auto testLane = createTestLane(
		Coordinates { 0.0, 0.0 },
		Coordinates { 0.001, 0.001 });
	agent.vehicle->current_lane = &testLane;

	// Set initial speed to 0
	agent.vehicle->currentSpeed = 0.0f;

	// Update time and run movement
	system->timeModule().update(0.016);
	system->vehicleMovementModule().update();

	// Verify speed was set to default (60.0f)
	EXPECT_EQ(agent.vehicle->currentSpeed, 60.0f);
}

TEST_F(VehicleMovementModuleTest, SpeedIsCappedAtMaxSpeed) {
	auto& agent = getAgent();

	// Set up agent with a goal
	agent.currentGoal = world.segments().front()->nodes.begin()->second.get();

	// Create a test lane
	auto testLane = createTestLane(
		Coordinates { 0.0, 0.0 },
		Coordinates { 0.001, 0.001 });
	agent.vehicle->current_lane = &testLane;

	// Set max speed lower than default speed
	agent.vehicle->maxSpeed = 30.0f;

	// Update time and run movement
	system->timeModule().update(0.016);
	system->vehicleMovementModule().update();

	// Verify speed was capped at max speed
	EXPECT_EQ(agent.vehicle->currentSpeed, 30.0f);
}

TEST_F(VehicleMovementModuleTest, RotationAngleIsCalculatedCorrectly) {
	auto& agent = getAgent();

	// Set up agent with a goal
	agent.currentGoal = world.segments().front()->nodes.begin()->second.get();

	// Create a test lane with known direction
	auto testLane = createTestLane(
		Coordinates { 0.0, 0.0 },
		Coordinates { 0.001, 0.001 } // 45-degree angle
	);
	agent.vehicle->current_lane = &testLane;

	// Update time and run movement
	system->timeModule().update(0.016);
	system->vehicleMovementModule().update();

	// Verify rotation angle was calculated (should be approximately 45 degrees)
	EXPECT_NEAR(agent.vehicle->rotationAngle, tjs::core::MathConstants::M_PI / 4.0, 0.1);
}

TEST_F(VehicleMovementModuleTest, LaneTransitionWhenReachingEnd) {
	auto& agent = getAgent();

	// Set up agent with a goal
	agent.currentGoal = world.segments().front()->nodes.begin()->second.get();

	// Create a test lane
	auto testLane = createTestLane(
		Coordinates { 0.0, 0.0 },
		Coordinates { 0.001, 0.001 });
	agent.vehicle->current_lane = &testLane;

	// Set s_on_lane to almost reach the end of the lane
	double laneLength = tjs::core::algo::haversine_distance(testLane.centerLine.front(), testLane.centerLine.back());
	agent.vehicle->s_on_lane = laneLength - 0.0001; // Almost at the end

	// Create a target lane
	auto targetLane = createTestLane(
		Coordinates { 0.001, 0.001 },
		Coordinates { 0.002, 0.002 });
	agent.target_lane = &targetLane;

	// Update time and run movement
	system->timeModule().update(0.016);
	system->vehicleMovementModule().update();

	// Verify lane transition occurred
	EXPECT_EQ(agent.vehicle->current_lane, &targetLane);
	EXPECT_EQ(agent.vehicle->s_on_lane, 0.0);
}

TEST_F(VehicleMovementModuleTest, NoLaneTransitionWhenNoTargetLane) {
	auto& agent = getAgent();

	// Set up agent with a goal
	agent.currentGoal = world.segments().front()->nodes.begin()->second.get();

	// Create a test lane
	auto testLane = createTestLane(
		Coordinates { 0.0, 0.0 },
		Coordinates { 0.001, 0.001 });
	agent.vehicle->current_lane = &testLane;

	// Set s_on_lane to almost reach the end of the lane
	double laneLength = tjs::core::algo::haversine_distance(testLane.centerLine.front(), testLane.centerLine.back());
	agent.vehicle->s_on_lane = laneLength - 0.0001; // Almost at the end

	// No target lane set
	agent.target_lane = nullptr;

	// Update time and run movement
	system->timeModule().update(0.016);
	system->vehicleMovementModule().update();

	// Verify no lane transition occurred
	EXPECT_EQ(agent.vehicle->current_lane, &testLane);
	EXPECT_EQ(agent.vehicle->s_on_lane, 0.0);
}

TEST_F(VehicleMovementModuleTest, MovementDistanceIsLimitedByLaneLength) {
	auto& agent = getAgent();

	// Set up agent with a goal
	agent.currentGoal = world.segments().front()->nodes.begin()->second.get();

	// Create a very short test lane
	auto testLane = createTestLane(
		Coordinates { 0.0, 0.0 },
		Coordinates { 0.0001, 0.0001 } // Very short lane
	);
	agent.vehicle->current_lane = &testLane;

	// Set s_on_lane to middle of lane
	double laneLength = tjs::core::algo::haversine_distance(testLane.centerLine.front(), testLane.centerLine.back());
	agent.vehicle->s_on_lane = laneLength / 2.0;

	// Update time with large delta to try to move beyond lane
	system->timeModule().update(1.0); // 1 second delta time
	system->vehicleMovementModule().update();

	// Verify s_on_lane doesn't exceed lane length
	EXPECT_LE(agent.vehicle->s_on_lane, laneLength);
}

TEST_F(VehicleMovementModuleTest, MultipleAgentsWithDifferentStates) {
	// Add a second vehicle and agent
	Vehicle v2 {};
	v2.uid = 2;
	v2.type = VehicleType::SimpleCar;
	v2.currentSpeed = 0.0f;
	v2.maxSpeed = 60.0f;
	v2.coordinates = Coordinates { 0.1, 0.1 };
	v2.currentWay = nullptr;
	v2.currentSegmentIndex = 0;
	v2.current_lane = nullptr;
	v2.s_on_lane = 0.0;
	v2.lateral_offset = 0.0;
	world.vehicles().push_back(v2);

	// Reinitialize system to include the new vehicle
	system->initialize();

	auto& agent1 = system->agents()[0];
	auto& agent2 = system->agents()[1];

	// Set up agent1 with goal and lane (should move)
	agent1.currentGoal = world.segments().front()->nodes.begin()->second.get();
	auto lane1 = createTestLane(Coordinates { 0.0, 0.0 }, Coordinates { 0.001, 0.001 });
	agent1.vehicle->current_lane = &lane1;

	// Set up agent2 with no goal (should not move)
	agent2.currentGoal = nullptr;
	auto lane2 = createTestLane(Coordinates { 0.1, 0.1 }, Coordinates { 0.101, 0.101 });
	agent2.vehicle->current_lane = &lane2;

	// Record initial positions
	Coordinates initialPos1 = agent1.vehicle->coordinates;
	Coordinates initialPos2 = agent2.vehicle->coordinates;

	// Update time and run movement
	system->timeModule().update(0.016);
	system->vehicleMovementModule().update();

	// Verify agent1 moved but agent2 didn't
	EXPECT_NE(agent1.vehicle->coordinates.x, initialPos1.x);
	EXPECT_NE(agent1.vehicle->coordinates.y, initialPos1.y);
	EXPECT_EQ(agent2.vehicle->coordinates.x, initialPos2.x);
	EXPECT_EQ(agent2.vehicle->coordinates.y, initialPos2.y);
}

#include "stdafx.h"

#include <core/data_layer/world_creator.h>
#include <core/simulation/simulation_system.h>
#include <core/simulation/movement/vehicle_movement_module.h>
#include <core/data_layer/road_network.h>
#include <core/math_constants.h>
#include <core/map_math/earth_math.h>
#include <core/data_layer/way_info.h>
#include <core/data_layer/lane_vehicle_utils.h>
#include <core/simulation/time_module.h>
#include <core/map_math/lane_connector_builder.h>

#include <simulation/simulation_tests_common.h>
#include <data_loader_mixin.h>

using namespace tjs::core;
using namespace tjs::core::simulation;

static Coordinates make_latlon(double lat, double lon) {
	Coordinates c {};
	c.latitude = lat;
	c.longitude = lon;
	c.x = lon * MathConstants::DEG_TO_RAD * MathConstants::EARTH_RADIUS;
	c.y = -std::log(std::tan((90.0 + lat) * MathConstants::DEG_TO_RAD / 2.0)) * MathConstants::EARTH_RADIUS;
	return c;
}

class VehicleMovementModuleTest : public ::tests::SimulationTestsCommon {
protected:
	virtual bool prepare() override {
		bool result = ::tests::SimulationTestsCommon::prepare();
		create_basic_system();
		algo::LaneConnectorBuilder::build_lane_connections(*get_segment().road_network);
		place_at_position();
		return result;
	}

	std::string default_map() const override {
		return "simple_grid.osmx";
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

	void setup_goal() {
		getAgent().currentGoal = world.segments().front()->nodes.begin()->second.get();
		// getAgent().vehicle->state = VehicleState::PendingMove;
	}

	void place_at_position(size_t way_idx = 0, size_t edge_idx = 0, size_t lane_idx = 0) {
		auto& agent = getAgent();
		auto& way = get_segment().ways.begin()->second;
		auto& edge = way->edges[0];
		agent.vehicle->current_lane = &edge->lanes[0];
		agent.vehicle->coordinates = edge->lanes[0].centerLine[0];
	}
};

TEST_F(VehicleMovementModuleTest, NoMovementWhenCurrentGoalIsNullptr) {
	auto& agent = getAgent();

	// Ensure agent has no current goal
	agent.currentGoal = nullptr;

	// Set up vehicle with a valid lane
	auto testLane = createTestLane(
		make_latlon(0.0, 0.0),
		make_latlon(0.001, 0.001));
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
	agent.currentGoal = get_segment().nodes.begin()->second.get();

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

	agent.currentGoal = get_segment().nodes.begin()->second.get();
	// Set up agent with a goal
	setup_goal();

	// set lower speed for way
	agent.vehicle->current_lane->parent->way->maxSpeed = 43;

	// Record initial position
	Coordinates initialPosition = agent.vehicle->coordinates;
	double initialSOnLane = agent.vehicle->s_on_lane;

	// Update time and run movement
	system->vehicleMovementModule().update();
	ASSERT_EQ(0, agent.vehicle->state_); // VehicleState::Moving
	system->vehicleMovementModule().update();

	// Verify movement occurred
	EXPECT_EQ(agent.vehicle->coordinates.x, initialPosition.x);
	EXPECT_NE(agent.vehicle->coordinates.y, initialPosition.y);
	EXPECT_GT(agent.vehicle->s_on_lane, initialSOnLane);

	// verify speed is set correctely - will be broken when accel will be added
	EXPECT_EQ(agent.vehicle->currentSpeed, agent.vehicle->current_lane->parent->way->maxSpeed);
}

TEST_F(VehicleMovementModuleTest, SpeedIsCappedAtMaxSpeed) {
	auto& agent = getAgent();

	agent.currentGoal = get_segment().nodes.begin()->second.get();
	// Set up agent with a goal
	setup_goal();

	// set lower speed for way and vehicle
	agent.vehicle->current_lane->parent->way->maxSpeed = 100;
	agent.vehicle->maxSpeed = 30;

	// Record initial position
	Coordinates initialPosition = agent.vehicle->coordinates;
	double initialSOnLane = agent.vehicle->s_on_lane;

	// Update time and run movement
	system->vehicleMovementModule().update();
	ASSERT_EQ(0, agent.vehicle->state_); // VehicleState::Moving
	system->vehicleMovementModule().update();

	// verify speed is set correctely - will be broken when accel will be added
	EXPECT_EQ(agent.vehicle->currentSpeed, 30);
}

TEST_F(VehicleMovementModuleTest, LaneChangeOccursWhenExceedingLaneLength) {
	auto& agent = getAgent();

	auto& segment = get_segment();
	ASSERT_FALSE(segment.ways.empty());
	auto& way = segment.ways.begin()->second;
	ASSERT_GE(way->edges.size(), 2u);
	auto& first_edge = *way->edges[0];
	auto& f_lane = first_edge.lanes[0];
	auto& second_edge = f_lane.outgoing_connections[0]->to->parent;

	agent.vehicle->current_lane = &first_edge.lanes[0];
	agent.vehicle->coordinates = first_edge.lanes[0].centerLine.front();
	insert_vehicle_sorted(*agent.vehicle->current_lane, agent.vehicle);

	agent.currentGoal = second_edge->end_node;
	agent.vehicle->state_ = 0; // VehicleState::Moving;
	agent.path.push_back(&(*second_edge));

	const double delta = (first_edge.lanes[0].length / (way->maxSpeed / 3.6)) + 10.0;
	const_cast<TimeState&>(system->timeModule().state()).set_fixed_delta(delta);
	system->vehicleMovementModule().update();

	EXPECT_EQ(agent.vehicle->current_lane->parent->get_id(), second_edge->get_id());
	EXPECT_TRUE(agent.path.empty());
	EXPECT_GT(agent.vehicle->s_on_lane, 0.0);
	EXPECT_TRUE(std::find(
					first_edge.lanes[0].vehicles.begin(),
					first_edge.lanes[0].vehicles.end(),
					agent.vehicle)
				== first_edge.lanes[0].vehicles.end());
	EXPECT_TRUE(std::find(
					second_edge->lanes[0].vehicles.begin(),
					second_edge->lanes[0].vehicles.end(),
					agent.vehicle)
				!= second_edge->lanes[0].vehicles.end());
}

TEST_F(VehicleMovementModuleTest, VehiclesRemainSortedAfterUpdate) {
	auto& agent = getAgent();

	auto& way = get_segment().ways.begin()->second;
	auto& edge = way->edges[0];
	auto& lane = edge->lanes[0];

	agent.vehicle->current_lane = &lane;
	agent.vehicle->coordinates = lane.centerLine.front();
	agent.vehicle->s_on_lane = 20.0;
	agent.vehicle->state_ = 0; //VehicleState::Moving;
	agent.currentGoal = lane.parent->end_node;

	Vehicle other {};
	other.uid = 2;
	other.current_lane = &lane;
	other.s_on_lane = 10.0;
	other.coordinates = lane.centerLine.front();

	lane.vehicles.push_back(agent.vehicle);
	lane.vehicles.push_back(&other); // intentionally unsorted

	system->timeModule().update(1.0);
	system->vehicleMovementModule().update();

	ASSERT_EQ(lane.vehicles.size(), 2u);
	EXPECT_LE(lane.vehicles[0]->s_on_lane, lane.vehicles[1]->s_on_lane);
}

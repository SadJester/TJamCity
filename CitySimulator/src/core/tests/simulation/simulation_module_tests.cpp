#include "stdafx.h"

#include <core/simulation/simulation_system.h>
#include <core/data_layer/world_creator.h>

#include <data_loader_mixin.h>
#include <simulation/simulation_tests_common.h>

using namespace tjs::core;
using namespace tjs::core::simulation;

class SimulationModuleTest : public ::tests::SimulationTestsCommon {
protected:
	void SetUp() override {
		ASSERT_TRUE(WorldCreator::loadOSMData(world, data_file("simple_grid.osmx").string()));

		create_basic_system();
	}
};

TEST_F(SimulationModuleTest, DISABLED_StrategicSetsGoal) {
	auto& agent = *system->agents()[0];
	EXPECT_EQ(agent.currentGoal, nullptr);
	system->strategicModule().update();
	EXPECT_NE(agent.currentGoal, nullptr);
}

TEST_F(SimulationModuleTest, DISABLED_TacticalBuildsPath) {
	auto& agent = *system->agents()[0];
	system->strategicModule().update();
	system->tacticalModule().update();
	EXPECT_GT(agent.path.size(), 0u);
}

TEST_F(SimulationModuleTest, DISABLED_VehicleMovesTowardsGoal) {
	auto& agent = *system->agents()[0];
	system->strategicModule().update();
	system->tacticalModule().update();
	Coordinates start = agent.vehicle->coordinates;
	system->timeModule().update(0.016);
	system->vehicleMovementModule().update();
	EXPECT_NE(start.x, agent.vehicle->coordinates.x);
	EXPECT_NE(start.y, agent.vehicle->coordinates.y);
}

TEST_F(SimulationModuleTest, DISABLED_TacticalMarksAgentStuckAfterFailures) {
	auto& agent = *system->agents()[0];
	auto unreachable = tjs::core::Node::create(9999, agent.vehicle->coordinates, tjs::core::NodeTags::None);

	agent.vehicle->state = 0; //VehicleState::Stopped;
	for (int i = 0; i < 5; ++i) {
		agent.currentGoal = unreachable.get();
		agent.path.clear();
		//agent.last_segment = false;
		system->tacticalModule().update();
	}

	EXPECT_TRUE(agent.stucked);
	EXPECT_GE(agent.goalFailCount, 5);
}

TEST_F(SimulationModuleTest, StrategicIgnoresStuckAgents) {
	auto& agent = *system->agents()[0];
	agent.stucked = true;
	agent.currentGoal = nullptr;
	system->strategicModule().update();
	EXPECT_EQ(agent.currentGoal, nullptr);
}

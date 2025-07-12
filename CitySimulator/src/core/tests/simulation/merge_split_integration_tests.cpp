#include "stdafx.h"

#include <core/simulation/simulation_system.h>
#include <core/data_layer/world_creator.h>

#include <data_loader_mixin.h>
#include <simulation/simulation_tests_common.h>

using namespace tjs::core;
using namespace tjs::core::simulation;

class MergeSplitIntegrationTest : public ::testing::Test, public ::tests::DataLoaderMixin, public ::tests::SimulationTestsCommon {
protected:
	void SetUp() override {
		ASSERT_TRUE(WorldCreator::loadOSMData(world, data_file("cross_merge.osmx").string()));
		create_basic_system();
	}
};

TEST_F(MergeSplitIntegrationTest, DISABLED_VehiclePassesMergeNode) {
	auto& agent = system->agents()[0];
	auto& network = *world.segments().front()->road_network;

	Edge* incoming = nullptr;
	Edge* outgoing = nullptr;
	for (auto& e : network.edges) {
		if (e.way->uid == 100) {
			incoming = &e;
		}
		if (e.way->uid == 101) {
			outgoing = &e;
		}
	}
	ASSERT_NE(incoming, nullptr);
	ASSERT_NE(outgoing, nullptr);

	Node* start = incoming->start_node;
	Node* goal = outgoing->end_node;

	agent.vehicle->coordinates = start->coordinates;
	agent.vehicle->current_lane = &incoming->lanes[0];
	//agent.target_lane = agent.vehicle->current_lane;
	agent.currentGoal = goal;

	system->tacticalModule().update();
	//ASSERT_EQ(agent.current_goal, incoming);
	ASSERT_EQ(agent.path.size(), 1u);
	EXPECT_EQ(agent.path.front(), outgoing);

	system->timeModule().update(70.0);
	system->vehicleMovementModule().update();

	system->tacticalModule().update();
	//EXPECT_EQ(agent.current_goal, outgoing);
	EXPECT_TRUE(agent.path.empty());

	system->timeModule().update(0.1);
	system->vehicleMovementModule().update();
	EXPECT_EQ(agent.vehicle->current_lane, &outgoing->lanes[0]);

	system->timeModule().update(70.0);
	system->vehicleMovementModule().update();

	system->tacticalModule().update();
	EXPECT_EQ(agent.currentGoal, nullptr);
}

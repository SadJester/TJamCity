#include "stdafx.h"

#include <data_loader_mixin.h>
#include <core/data_layer/world_creator.h>
#include <core/data_layer/world_data.h>

using namespace tjs::core;

class MergeSplitTest : public ::testing::Test, tjs::core::tests::DataLoaderMixin {
protected:
	WorldData world;
	void SetUp() override {
		ASSERT_TRUE(WorldCreator::loadOSMData(world, data_file("cross_merge.osmx").string()));
	}
};

TEST_F(MergeSplitTest, LaneToLaneConnectionsCreated) {
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

	ASSERT_EQ(incoming->lanes.size(), 2u);
	ASSERT_EQ(outgoing->lanes.size(), 2u);

	for (size_t i = 0; i < 2; ++i) {
		const Lane& in_lane = incoming->lanes[i];
		ASSERT_EQ(in_lane.outgoing_connections.size(), 1u);
		const LaneLinkHandler& link = in_lane.outgoing_connections.front();
		EXPECT_EQ(link->to, &outgoing->lanes[i]);
		EXPECT_TRUE(link->yield); // incoming way is _link
	}
}

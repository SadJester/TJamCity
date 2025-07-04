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

class RampMergeTest : public ::testing::Test, tjs::core::tests::DataLoaderMixin {
protected:
	WorldData world;
	void SetUp() override {
		ASSERT_TRUE(WorldCreator::loadOSMData(world, data_file("ramp_merge.osmx").string()));
	}
};

class LaneMismatchTest : public ::testing::Test, tjs::core::tests::DataLoaderMixin {
protected:
	WorldData world;
	void SetUp() override {
		ASSERT_TRUE(WorldCreator::loadOSMData(world, data_file("lane_mismatch.osmx").string()));
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

TEST_F(RampMergeTest, DISABLED_MergesIntoRightmostLane) {
	auto& network = *world.segments().front()->road_network;

	Edge* ramp = nullptr;
	Edge* mainline = nullptr;
	for (auto& e : network.edges) {
		if (e.way->uid == 100) {
			ramp = &e;
		}
		if (e.way->uid == 101) {
			mainline = &e;
		}
	}
	ASSERT_NE(ramp, nullptr);
	ASSERT_NE(mainline, nullptr);
	ASSERT_EQ(ramp->lanes.size(), 1u);
	ASSERT_EQ(mainline->lanes.size(), 4u);

	const Lane& in_lane = ramp->lanes.front();
	ASSERT_EQ(in_lane.outgoing_connections.size(), 1u);
	const LaneLinkHandler& link = in_lane.outgoing_connections.front();
	EXPECT_EQ(link->to, &mainline->lanes.back());
	EXPECT_TRUE(link->yield); // ramp is a link road
}

TEST_F(LaneMismatchTest, ChoosesClosestLaneByPosition) {
	auto& network = *world.segments().front()->road_network;

	Edge* in_edge = nullptr;
	Edge* out_edge = nullptr;
	for (auto& e : network.edges) {
		if (e.way->uid == 200) {
			in_edge = &e;
		}
		if (e.way->uid == 201) {
			out_edge = &e;
		}
	}
	ASSERT_NE(in_edge, nullptr);
	ASSERT_NE(out_edge, nullptr);
	ASSERT_EQ(in_edge->lanes.size(), 3u);
	ASSERT_EQ(out_edge->lanes.size(), 2u);

	// leftmost lane maps to left lane
	const Lane& left = in_edge->lanes.front();
	ASSERT_EQ(left.outgoing_connections.size(), 1u);
	EXPECT_EQ(left.outgoing_connections.front()->to, &out_edge->lanes.front());

	// middle lane maps to right lane (closest)
	const Lane& middle = in_edge->lanes[1];
	ASSERT_EQ(middle.outgoing_connections.size(), 1u);
	EXPECT_EQ(middle.outgoing_connections.front()->to, &out_edge->lanes.back());

	// rightmost lane also maps to right lane
	const Lane& right = in_edge->lanes.back();
	ASSERT_EQ(right.outgoing_connections.size(), 1u);
	EXPECT_EQ(right.outgoing_connections.front()->to, &out_edge->lanes.back());
}

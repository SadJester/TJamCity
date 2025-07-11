#include "stdafx.h"

#include <data_loader_mixin.h>
#include <core/data_layer/world_creator.h>
#include <core/data_layer/world_data.h>
#include <core/map_math/path_finder.h>

using namespace tjs::core;

class PathFinderTest : public ::testing::Test, tjs::core::tests::DataLoaderMixin {
protected:
	WorldData world;

	void SetUp() override {
		ASSERT_TRUE(WorldCreator::loadOSMData(world, data_file("cross_junction.osmx").string()));
	}
};

class ComplexStreetsTest : public ::testing::Test, tjs::core::tests::DataLoaderMixin {
protected:
	WorldData world;

	void SetUp() override {
		ASSERT_TRUE(WorldCreator::loadOSMData(world, data_file("complex_streets.osmx").string()));
	}
};

TEST_F(PathFinderTest, CrossJunctionPath) {
	auto& network = *world.segments().front()->road_network;
	Node* start = network.nodes.at(2);
	Node* target = network.nodes.at(5);

	auto path = algo::PathFinder::find_path_a_star(network, start, target);

	ASSERT_EQ(path.front(), start);
	ASSERT_EQ(path.back(), target);
	EXPECT_EQ(path.size(), 3u);
}

TEST_F(PathFinderTest, DISABLED_ReverseDirection) {
	auto& network = *world.segments().front()->road_network;
	Node* start = network.nodes.at(5);
	Node* target = network.nodes.at(3);

	auto path = algo::PathFinder::find_path_a_star(network, start, target);

	ASSERT_FALSE(path.empty());
	ASSERT_EQ(path.front(), start);
	ASSERT_EQ(path.back(), target);
	EXPECT_EQ(path.size(), 3u);
}

TEST_F(ComplexStreetsTest, CornerToCornerPath) {
	auto& network = *world.segments().front()->road_network;
	Node* start = network.nodes.at(1);
	Node* target = network.nodes.at(16);

	auto path = algo::PathFinder::find_path_a_star(network, start, target);

	ASSERT_EQ(path.front(), start);
	ASSERT_EQ(path.back(), target);
	EXPECT_GE(path.size(), 4u);
}

TEST_F(PathFinderTest, DISABLED_ReachableNodes) {
	auto& network = *world.segments().front()->road_network;
	Node* start = network.nodes.at(1);
	auto reachable = algo::PathFinder::reachable_nodes(network, start);
	EXPECT_EQ(reachable.size(), 5u);
	EXPECT_TRUE(reachable.contains(network.nodes.at(5)));
}

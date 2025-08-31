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

class ChicagoGridTest : public ::testing::Test, tjs::core::tests::DataLoaderMixin {
protected:
	WorldData world;

	void SetUp() override {
		ASSERT_TRUE(
			WorldCreator::loadOSMData(world, sample_file("chicago_like_grid.osmx").string()));
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

TEST_F(ChicagoGridTest, EdgePathFromLane) {
	auto& network = *world.segments().front()->road_network;

	const Lane* start_lane = nullptr;
	const Edge* first_edge = nullptr;
	Node* target = nullptr;
	std::vector<const Edge*> tail;

	for (auto& edge : network.edges) {
		for (auto& lane : edge.lanes) {
			if (!lane.outgoing_connections.empty()) {
				const LaneLinkHandler& link_h = lane.outgoing_connections.front();
				const LaneLink& link = *link_h;
				if (link.to == nullptr) {
					continue;
				}
				const Edge* next_edge = link.to->parent;
				Node* start_node = next_edge->end_node;
				Node* candidate_target = network.edges.back().end_node;
				if (candidate_target == start_node && network.edges.size() > 1) {
					candidate_target = network.edges[network.edges.size() - 2].end_node;
				}
				auto candidate_tail = algo::PathFinder::find_edge_path_a_star(
					network, start_node, candidate_target);
				if (!candidate_tail.empty()) {
					start_lane = &lane;
					first_edge = next_edge;
					target = candidate_target;
					tail = std::move(candidate_tail);
					break;
				}
			}
		}
		if (start_lane != nullptr) {
			break;
		}
	}

	ASSERT_NE(start_lane, nullptr);
	ASSERT_NE(first_edge, nullptr);
	ASSERT_NE(target, nullptr);

	std::vector<const Edge*> expected_path;
	expected_path.push_back(first_edge);
	expected_path.insert(expected_path.end(), tail.begin(), tail.end());

	auto path = algo::PathFinder::find_edge_path_a_star_from_lane(network, start_lane, target, false);

	ASSERT_EQ(path.size(), expected_path.size());
	for (size_t i = 0; i < path.size(); ++i) {
		EXPECT_EQ(path[i], expected_path[i]);
	}

	auto path_with_adjacent =
		algo::PathFinder::find_edge_path_a_star_from_lane(network, start_lane, target, true);

	ASSERT_EQ(path_with_adjacent.size(), expected_path.size());
	for (size_t i = 0; i < path_with_adjacent.size(); ++i) {
		EXPECT_EQ(path_with_adjacent[i], expected_path[i]);
	}
}

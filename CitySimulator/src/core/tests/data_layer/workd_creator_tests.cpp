#include <stdafx.h>

#include <core/data_layer/world_data.h>
#include <core/data_layer/world_creator.h>
#include <core/data_layer/data_types.h>
#include <core/map_math/contraction_builder.h>
#include <core/map_math/lane_connector_builder.h>

#include <data_loader_mixin.h>

using namespace tjs::core;

class WorldCreatorTests
	: public ::testing::Test,
	  public ::tests::DataLoaderMixin {
protected:
	void SetUp() override {
		tjs::core::Lane::reset_id();
		tjs::core::Edge::reset_id();
		ASSERT_TRUE(WorldCreator::loadOSMData(world, data_file("test_lanes.osmx").string()));
		ASSERT_TRUE(world.segments().size() == 1);

		details::preprocess_segment(*world.segments()[0]);

		algo::ContractionBuilder builder;
		builder.build_graph(*world.segments()[0]->road_network);
	}

	Node* get_node(uint64_t id) {
		return world.segments()[0]->nodes[id].get();
	}

	WayInfo* get_way(uint64_t id) {
		return world.segments()[0]->ways[id].get();
	}

	RoadNetwork& get_road_network() {
		return *world.segments()[0]->road_network;
	}

	const std::vector<LaneLinkHandler>& outgoing(Edge& edge, size_t index) {
		return edge.lanes[index].outgoing_connections;
	}

	const std::vector<LaneLinkHandler>& incoming(Edge& edge, size_t index) {
		return edge.lanes[index].incoming_connections;
	}

protected:
	tjs::core::WorldData world;
};

TEST_F(WorldCreatorTests, CreateEdge_WayToTop_RightLane_MaxX) {
	Node* start_node = get_node(6015664834);
	Node* end_node = get_node(1496468807);
	WayInfo* way = get_way(261198607);

	ASSERT_NE(start_node, nullptr);
	ASSERT_NE(end_node, nullptr);
	ASSERT_NE(way, nullptr);

	Edge edge = algo::create_edge(
		start_node,
		end_node,
		way,
		0.0,
		LaneOrientation::Forward);

	ASSERT_EQ(edge.lanes.size(), 2u);

	ASSERT_GE(edge.lanes[0].centerLine[0].x, edge.lanes[1].centerLine[0].x);
}

TEST_F(WorldCreatorTests, CreateEdge_MergingLanes_2x2_to_4) {
	Node* node = get_node(1496468807);
	auto& network = get_road_network();

	ASSERT_NE(node, nullptr);

	algo::details::process_node(network, node);

	auto adjacent = algo::details::get_adjacent_edges(network, node);

	Edge& right_merge_edge = *adjacent.incoming[0];
	Edge& left_merge_edge = *adjacent.incoming[1];
	Edge& out_edge = *adjacent.outgoing[0];

	const size_t first_outgoing_idx = out_edge.lanes[0].get_id();
	// 6015664834[0] -> 1496468807[0]
	ASSERT_EQ(1, outgoing(right_merge_edge, 0).size());
	ASSERT_EQ(first_outgoing_idx, outgoing(right_merge_edge, 0)[0]->to->get_id());
	EXPECT_EQ(1, incoming(out_edge, 0).size());
	EXPECT_EQ(outgoing(right_merge_edge, 0)[0]->from, incoming(out_edge, 0)[0]->from);

	// 6015664834[1] -> 1496468807[1]
	ASSERT_EQ(1, outgoing(right_merge_edge, 1).size());
	ASSERT_EQ(first_outgoing_idx + 1, outgoing(right_merge_edge, 1)[0]->to->get_id());
	EXPECT_EQ(1, incoming(out_edge, 1).size());
	EXPECT_EQ(outgoing(right_merge_edge, 1)[0]->from, incoming(out_edge, 1)[0]->from);

	// 1496468775[0] -> 1496468807[2]
	ASSERT_EQ(1, outgoing(left_merge_edge, 0).size());
	ASSERT_EQ(first_outgoing_idx + 2, outgoing(left_merge_edge, 0)[0]->to->get_id());
	EXPECT_EQ(1, incoming(out_edge, 2).size());
	EXPECT_EQ(outgoing(left_merge_edge, 0)[0]->from, incoming(out_edge, 2)[0]->from);

	// 1496468775[1] -> 1496468807[3]
	ASSERT_EQ(1, outgoing(left_merge_edge, 1).size());
	ASSERT_EQ(first_outgoing_idx + 3, outgoing(left_merge_edge, 1)[0]->to->get_id());
	EXPECT_EQ(1, incoming(out_edge, 3).size());
	EXPECT_EQ(outgoing(left_merge_edge, 1)[0]->from, incoming(out_edge, 3)[0]->from);
}

TEST_F(WorldCreatorTests, CreateEdge_Straight_4x4) {
	Node* node = get_node(1527931002);
	auto& network = get_road_network();

	ASSERT_NE(node, nullptr);

	algo::details::process_node(network, node);

	auto adjacent = algo::details::get_adjacent_edges(network, node);
	Edge& in_edge = *adjacent.incoming[0];
	Edge& out_edge = *adjacent.outgoing[0];

	ASSERT_EQ(4, in_edge.lanes.size());
	ASSERT_EQ(4, out_edge.lanes.size());

	const size_t first_outgoing_idx = out_edge.lanes[0].get_id();
	for (size_t i = 0; i < 4; ++i) {
		ASSERT_EQ(1, outgoing(in_edge, i).size());
		ASSERT_EQ(first_outgoing_idx + i, outgoing(in_edge, i)[0]->to->get_id());
		ASSERT_EQ(1, incoming(out_edge, i).size());
		ASSERT_EQ(outgoing(in_edge, i)[0]->from, incoming(out_edge, i)[0]->from);
	}
}

TEST_F(WorldCreatorTests, CreateEdge_4_to_3) {
	Node* node = get_node(2702442374);
	auto& network = get_road_network();

	ASSERT_NE(node, nullptr);

	algo::details::process_node(network, node);

	auto adjacent = algo::details::get_adjacent_edges(network, node);
	Edge& in_edge = *adjacent.incoming[0];
	Edge& out_edge = *adjacent.outgoing[0];

	ASSERT_EQ(4, in_edge.lanes.size());
	ASSERT_EQ(3, out_edge.lanes.size());

	const size_t first_outgoing_idx = out_edge.lanes[0].get_id();

	ASSERT_EQ(1, outgoing(in_edge, 0).size());
	ASSERT_EQ(first_outgoing_idx, outgoing(in_edge, 0)[0]->to->get_id());

	ASSERT_EQ(2, incoming(out_edge, 0).size());
	ASSERT_EQ(outgoing(in_edge, 0)[0]->from, incoming(out_edge, 0)[0]->from);
	ASSERT_EQ(outgoing(in_edge, 1)[0]->from, incoming(out_edge, 0)[1]->from);

	for (size_t i = 1; i < 3; ++i) {
		// There is shift + 1 because 0 lane has no connection
		const size_t in_edge_idx = i + 1;
		ASSERT_EQ(1, outgoing(in_edge, in_edge_idx).size());
		ASSERT_EQ(first_outgoing_idx + i, outgoing(in_edge, in_edge_idx)[0]->to->get_id());
		ASSERT_EQ(1, incoming(out_edge, i).size());
		ASSERT_EQ(outgoing(in_edge, in_edge_idx)[0]->from, incoming(out_edge, i)[0]->from);
	}
}

TEST_F(WorldCreatorTests, reateEdge_T_Shaped) {
	Node* node = get_node(1527930956);
	auto& network = get_road_network();

	ASSERT_NE(node, nullptr);

	algo::details::process_node(network, node);

	auto adjacent = algo::details::get_adjacent_edges(network, node);
	Edge& primary_in = *adjacent.incoming[1];
	Edge& secondary_in = *adjacent.incoming[0];
	Edge& primary_out = *adjacent.outgoing[1];
	Edge& secondary_out = *adjacent.outgoing[0];

	ASSERT_EQ(3, primary_out.lanes.size());
	ASSERT_EQ(3, primary_in.lanes.size());

	// primary connections
	const size_t first_primary_idx = primary_out.lanes[0].get_id();

	size_t lane_idx = 0;
	ASSERT_EQ(2, outgoing(primary_in, lane_idx).size());
	ASSERT_EQ(first_primary_idx, outgoing(primary_in, lane_idx)[1]->to->get_id());
	ASSERT_EQ(2, incoming(primary_out, lane_idx).size());
	ASSERT_EQ(outgoing(primary_in, lane_idx)[1]->from, incoming(primary_out, lane_idx)[1]->from);

	lane_idx = 1;
	ASSERT_EQ(1, outgoing(primary_in, lane_idx).size());
	ASSERT_EQ(first_primary_idx + 1, outgoing(primary_in, lane_idx)[0]->to->get_id());
	ASSERT_EQ(1, incoming(primary_out, lane_idx).size());
	ASSERT_EQ(outgoing(primary_in, lane_idx)[0]->from, incoming(primary_out, lane_idx)[0]->from);

	lane_idx = 2;
	ASSERT_EQ(1, outgoing(primary_in, lane_idx).size());
	ASSERT_EQ(first_primary_idx + 2, outgoing(primary_in, lane_idx)[0]->to->get_id());
	ASSERT_EQ(1, incoming(primary_out, lane_idx).size());
	ASSERT_EQ(outgoing(primary_in, lane_idx)[0]->from, incoming(primary_out, lane_idx)[0]->from);

	// secondary in primary right
	lane_idx = 0;
	ASSERT_EQ(1, outgoing(secondary_in, lane_idx).size());
	ASSERT_EQ(first_primary_idx, outgoing(secondary_in, lane_idx)[0]->to->get_id());
	ASSERT_EQ(2, incoming(primary_out, lane_idx).size());
	ASSERT_EQ(outgoing(secondary_in, lane_idx)[0]->from, incoming(primary_out, lane_idx)[0]->from);

	// primary right to secondary out
	lane_idx = 0;
	const size_t secondary_lane_idx = secondary_out.lanes[0].get_id();
	ASSERT_EQ(secondary_lane_idx, outgoing(primary_in, lane_idx)[0]->to->get_id());
	ASSERT_EQ(1, incoming(secondary_out, lane_idx).size());
	ASSERT_EQ(outgoing(primary_in, lane_idx)[0]->from, incoming(secondary_out, lane_idx)[0]->from);
}

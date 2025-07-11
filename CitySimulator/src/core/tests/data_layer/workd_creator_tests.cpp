#include <stdafx.h>

#include <core/data_layer/world_data.h>
#include <core/data_layer/world_creator.h>
#include <core/data_layer/data_types.h>
#include <core/map_math/contraction_builder.h>
#include <core/map_math/lane_connector_builder.h>

#include <data_loader_mixin.h>
#include <data_layer/world_utils.h>

using namespace tjs;
using namespace tjs::core;

class WorldCreatorTests
	: public ::testing::Test,
	  public ::tests::DataLoaderMixin {
protected:
	void SetUp() override {
		Lane::reset_id();
		Edge::reset_id();
		ASSERT_TRUE(load_map());
		ASSERT_TRUE(prepare());
	}

	virtual bool prepare() {
		details::preprocess_segment(*world.segments()[0]);

		algo::ContractionBuilder builder;
		builder.build_graph(*world.segments()[0]->road_network);

		return true;
	}

	virtual bool load_map() {
		bool result = WorldCreator::loadOSMData(world, data_file("test_lanes.osmx").string());
		return world.segments().size() == 1;
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

class BaseLaneFixture : public ::testing::Test {
protected:
	// constants
	static constexpr double kW = 3.0; // lane width [m]

	// nodes reused by every test
	tjs::core::Node n0_ {}, n1_ {};

	// helper: analytic offset of global lane G (right-most = 0)
	static double expected_offset(std::size_t g,
		std::size_t total,
		double lane_w) {
		const double half = static_cast<double>(total - 1) / 2.0;
		return (static_cast<double>(g) - half) * lane_w;
	}

	void SetUp() override {
		tjs::core::Lane::reset_id();
		tjs::core::Edge::reset_id();
		n0_.coordinates = { 0.0, 0.0, 0.0, 10.0 }; // straight north-bound edge
		n1_.coordinates = { 0.0, 0.0, 0.0, 0.0 };
	}
};

class ForwardParamTest
	: public BaseLaneFixture,
	  public ::testing::WithParamInterface<std::size_t> // lanes
{};

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

// ───────────────────────────────────────────────────────────────────────────
// 1. Forward-only edges (1 … 5 lanes)
// ───────────────────────────────────────────────────────────────────────────
TEST_P(ForwardParamTest, CreateEdge_CheckOffsetAndTurn) {
	const std::size_t lanes = GetParam();

	auto way = core::tests::make_way(1000 + lanes, lanes, 0, kW);

	core::Edge e = algo::create_edge(&n0_, &n1_,
		way.get(), /*dist=*/100.0,
		LaneOrientation::Forward);

	ASSERT_EQ(e.lanes.size(), lanes);

	for (std::size_t i = 0; i < lanes; ++i) {
		const auto& L = e.lanes[i];
		double exp_x = expected_offset(i, lanes, kW);

		EXPECT_NEAR(L.centerLine.front().x, exp_x, 1e-6);
		EXPECT_NEAR(L.centerLine.back().x, exp_x, 1e-6);
		EXPECT_EQ(L.turn,
			way->forwardTurns[lanes - 1 - i]); // td_index rule
	}
}

// Instantiate for 1..5 lanes
INSTANTIATE_TEST_SUITE_P(ForwardSizes,
	ForwardParamTest,
	::testing::Values(1u, 2u, 3u, 4u, 5u));

// ───────────────────────────────────────────────────────────────────────────
// 2. Mixed direction cases
// ───────────────────────────────────────────────────────────────────────────
struct MixCfg {
	std::size_t fwd, back;
};
std::ostream& operator<<(std::ostream& os, const MixCfg& m) { return os << m.fwd << 'F' << '_' << m.back << 'B'; }

class MixedParamTest
	: public BaseLaneFixture,
	  public ::testing::WithParamInterface<MixCfg> {};

TEST_P(MixedParamTest, CreateEdge_MixedDirections_WayDown) {
	const auto cfg = GetParam();

	auto way = core::tests::make_way(2000 + cfg.fwd * 10 + cfg.back,
		cfg.fwd, cfg.back, kW);

	const size_t total = cfg.fwd + cfg.back;

	// ---------- Forward edge ----------
	core::Edge ef = algo::create_edge(&n0_, &n1_,
		way.get(), 100.0,
		LaneOrientation::Forward);

	ASSERT_EQ(ef.lanes.size(), cfg.fwd);
	for (std::size_t i = 0; i < cfg.fwd; ++i) {
		double exp_x = expected_offset(i, total, kW);
		EXPECT_NEAR(ef.lanes[i].centerLine.front().x, exp_x, 1e-6);
		EXPECT_EQ(ef.lanes[i].turn, way->forwardTurns[cfg.fwd - 1 - i]);
	}

	for (size_t i = 0; i < cfg.fwd - 1; ++i) {
		EXPECT_LT(ef.lanes[i].centerLine.front().x, ef.lanes[i + 1].centerLine.front().x);
	}

	// ---------- Backward edge (most right is greater x) ----------
	core::Edge eb = algo::create_edge(&n1_, &n0_,
		way.get(), 100.0,
		LaneOrientation::Backward);

	ASSERT_EQ(eb.lanes.size(), cfg.back);
	for (std::size_t i = 0; i < cfg.back; ++i) {
		std::size_t global_idx = cfg.fwd + i; // after all F lanes
		double exp_x = expected_offset(global_idx, total, kW);

		//EXPECT_NEAR(eb.lanes[i].centerLine.front().x, exp_x, 1e-6);
		EXPECT_EQ(eb.lanes[i].turn, way->backwardTurns[cfg.back - i - 1]);
	}

	EXPECT_LT(ef.lanes[ef.lanes.size() - 1].centerLine.front().x, eb.lanes[0].centerLine.front().x);
	for (size_t i = 0; i < cfg.back - 1; ++i) {
		EXPECT_GT(eb.lanes[i].centerLine.front().x, eb.lanes[i + 1].centerLine.front().x);
	}
}

TEST_P(MixedParamTest, CreateEdge_MixedDirections_WayUp) {
	const auto cfg = GetParam();

	auto way = core::tests::make_way(2000 + cfg.fwd * 10 + cfg.back,
		cfg.fwd, cfg.back, kW);

	const size_t total = cfg.fwd + cfg.back;

	// ---------- Forward edge ----------
	core::Edge ef = algo::create_edge(&n1_, &n0_,
		way.get(), 100.0,
		LaneOrientation::Forward);

	ASSERT_EQ(ef.lanes.size(), cfg.fwd);
	for (std::size_t i = 0; i < cfg.fwd; ++i) {
		size_t chnage_idx = cfg.fwd - i - 1;

		double exp_x = expected_offset(chnage_idx, total, kW);
		EXPECT_NEAR(ef.lanes[i].centerLine.front().x, exp_x, 1e-6);
		EXPECT_EQ(ef.lanes[i].turn, way->forwardTurns[cfg.fwd - 1 - i]);
	}

	for (size_t i = 0; i < cfg.fwd - 1; ++i) {
		EXPECT_GT(ef.lanes[i].centerLine.front().x, ef.lanes[i + 1].centerLine.front().x);
	}

	// ---------- Backward edge from vise verca y (most right is less x) ----------
	core::Edge eb = algo::create_edge(&n0_, &n1_,
		way.get(), 100.0,
		LaneOrientation::Backward);

	ASSERT_EQ(eb.lanes.size(), cfg.back);
	for (std::size_t i = 0; i < cfg.back; ++i) {
		size_t chnage_idx = cfg.back - i - 1;
		std::size_t global_idx = cfg.fwd + i; // after all F lanes
		double exp_x = expected_offset(i, total, kW);

		//EXPECT_NEAR(eb.lanes[i].centerLine.front().x, exp_x, 1e-6);
		EXPECT_EQ(eb.lanes[i].turn, way->backwardTurns[chnage_idx]); // td_index rule
	}

	EXPECT_LT(ef.lanes[0].centerLine.front().x, eb.lanes[0].centerLine.front().x);
	for (size_t i = 0; i < cfg.back - 1; ++i) {
		EXPECT_LT(eb.lanes[i].centerLine.front().x, eb.lanes[i + 1].centerLine.front().x);
	}
}

INSTANTIATE_TEST_SUITE_P(MixedSets,
	MixedParamTest,
	::testing::Values(MixCfg { 2, 1 },
		MixCfg { 1, 2 },
		MixCfg { 2, 2 }));

// ───────────────────────────────────────────────────────────────────────────
// Create lanes tests
// ───────────────────────────────────────────────────────────────────────────

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

TEST_F(WorldCreatorTests, CreateEdge_T_Shaped) {
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

// ───────────────────────────────────────────────────────────────────────────
// Create lanes tests - samples
// ───────────────────────────────────────────────────────────────────────────

class CreateLanesTests : public WorldCreatorTests {
public:
	using IntVec = std::vector<int>;
	struct LaneLinks {
		int from;

		IntVec incoming;
		IntVec outgoing;

		LaneLinks(int from_, IntVec&& inc, IntVec&& out)
			: from(from_)
			, incoming(inc)
			, outgoing(out) {
		}
	};

public:
	bool load_map() override {
		bool result = core::details::loadOSMXmlData(world, sample_file("grid_osm_equivalent.osmx").string());
		return world.segments().size() == 1;
	}

	bool prepare() override {
		core::details::preprocess_segment(*world.segments().front());
		algo::ContractionBuilder builder;
		builder.build_graph(*world.segments().front()->road_network);
		return true;
	}

	std::optional<const Lane*> get_lane(int id, const algo::details::AdjacentEdges& adjacent) {
		for (auto edge : adjacent.incoming) {
			for (const auto& lane : edge->lanes) {
				if (lane.get_id() == id) {
					return &lane;
				}
			}
		}

		for (auto edge : adjacent.outgoing) {
			for (const auto& lane : edge->lanes) {
				if (lane.get_id() == id) {
					return &lane;
				}
			}
		}

		return std::nullopt;
	}

	void check_outgoing(int test_id, const LaneLinks& links, const Lane& lane) {
		const std::vector<LaneLinkHandler>& connections = lane.outgoing_connections;
		auto& ids = links.outgoing;

		EXPECT_EQ(ids.size(), connections.size());
		if (ids.size() != connections.size()) {
			return;
		}
		for (size_t i = 0; i < ids.size(); ++i) {
			EXPECT_EQ(test_id, connections[i]->from->get_id());
			EXPECT_EQ(ids[i], connections[i]->to->get_id());
		}
	}

	void check_incoming(int test_id, const LaneLinks& links, const Lane& lane) {
		const std::vector<LaneLinkHandler>& connections = lane.incoming_connections;
		auto& ids = links.incoming;

		EXPECT_EQ(ids.size(), connections.size());
		if (ids.size() != connections.size()) {
			return;
		}
		for (size_t i = 0; i < ids.size(); ++i) {
			EXPECT_EQ(test_id, connections[i]->to->get_id());
			EXPECT_EQ(ids[i], connections[i]->from->get_id());
		}
	}

	void check_lane(const algo::details::AdjacentEdges& adjacent, const LaneLinks& link) {
		auto lane = get_lane(link.from, adjacent);
		ASSERT_TRUE(lane.has_value());

		ASSERT_EQ(link.incoming.size(), (*lane)->incoming_connections.size()) << "Unexpected incoming size for lane \"" << (*lane)->get_id()
																			  << "\". Expected: " << link.incoming.size() << ". Actual: " << (*lane)->incoming_connections.size();
		ASSERT_EQ(link.outgoing.size(), (*lane)->outgoing_connections.size()) << "Unexpected outgoing size for " << (*lane)->get_id()
																			  << " Expected: " << link.outgoing.size() << ". Actual: " << (*lane)->outgoing_connections.size();

		check_incoming(link.from, link, *(*lane));
		check_outgoing(link.from, link, *(*lane));
	}
};

TEST_F(CreateLanesTests, ProcessLanes_CornerNode) {
	Node* node = get_node(1);
	auto& network = get_road_network();

	ASSERT_NE(node, nullptr);

	algo::details::process_node(network, node);

	const auto adjacent = algo::details::get_adjacent_edges(network, node);

	check_lane(adjacent, { 62,
							 {},
							 { 1 } });

	check_lane(adjacent, {
							 2,
							 {},
							 { 60 },
						 });
}

TEST_F(CreateLanesTests, ProcessLanes_LeftMiddle) {
	Node* node = get_node(2);
	auto& network = get_road_network();

	ASSERT_NE(node, nullptr);

	algo::details::process_node(network, node);
	// TODO: check no duplicates
	// algo::details::process_node(network, node);

	const auto adjacent = algo::details::get_adjacent_edges(network, node);

	check_lane(adjacent, { 0,
							 {},
							 { 3 } });

	check_lane(adjacent, {
							 1,
							 {},
							 { 73, 4 },
						 });

	check_lane(adjacent, {
							 2,
							 { 74, 5 },
							 {},
						 });
}

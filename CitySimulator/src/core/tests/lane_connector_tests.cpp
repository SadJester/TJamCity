#include "stdafx.h"

#include <core/data_layer/world_creator.h>
#include <core/data_layer/world_data.h>

#include <filesystem>

using namespace tjs::core;

namespace {
	std::filesystem::path data_file(const char* name) {
		return std::filesystem::path(__FILE__).parent_path() / "test_data" / name;
	}
} // namespace

class LaneConnectorTest : public ::testing::Test {
protected:
	WorldData world;
	void SetUp() override {
		ASSERT_TRUE(WorldCreator::loadOSMData(world, data_file("cross_junction.osmx").string()));
	}
};

TEST_F(LaneConnectorTest, ConnectionsCreated) {
	auto& network = *world.segments().front()->road_network;
	Node* start = network.nodes.at(2);
	Node* junction = network.nodes.at(1);
	Edge* incoming = nullptr;
	for (auto& e : network.edges) {
		if (e.start_node == start && e.end_node == junction) {
			incoming = &e;
			break;
		}
	}
	ASSERT_NE(incoming, nullptr);
	ASSERT_FALSE(incoming->lanes.empty());
	const auto& lane = incoming->lanes.front();
	EXPECT_GT(lane.outgoing_connections.size(), 0u);
}

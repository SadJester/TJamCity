#include <stdafx.h>

#include <core/dataLayer/data_types.h>


using namespace tjs::core;


class SpatialGridTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test nodes
        node1 = std::make_unique<Node>();
        node1->uid = 1;
        node1->coordinates = {10.0, 20.0};
        
        node2 = std::make_unique<Node>();
        node2->uid = 2;
        node2->coordinates = {15.0, 25.0};

        node3 = std::make_unique<Node>();
        node3->uid = 3;
        node3->coordinates = {11.5, 22.5};
        
        // Create test ways
        way1 = std::make_unique<WayInfo>();
        way1->uid = 1;
        way1->nodes.push_back(node1.get());
        way1->nodes.push_back(node2.get());
        
        way2 = std::make_unique<WayInfo>();
        way2->uid = 2;
        way2->nodes.push_back(node1.get());
        way2->nodes.push_back(node3.get());
        
        // Configure grid with 1.0 cell size for precise testing
        grid.cellSize = 1.0;
    }

    SpatialGrid grid;
    std::unique_ptr<Node> node1, node2, node3;
    std::unique_ptr<WayInfo> way1, way2;
};

/*
=======================================
Basic functionality
======================================
*/
TEST_F(SpatialGridTest, AddSingleWay) {
    grid.add_way(way1.get());
    
    // Should be added to two cells (for node1 and node2)
    EXPECT_EQ(grid.spatialGrid.size(), 2);
    
    // Check node1's cell
    auto key1 = std::make_pair(10, 20);
    ASSERT_TRUE(grid.spatialGrid.count(key1));
    EXPECT_EQ(grid.spatialGrid[key1].size(), 1);
    EXPECT_EQ(grid.spatialGrid[key1][0], way1.get());
    
    // Check node2's cell
    auto key2 = std::make_pair(15, 25);
    ASSERT_TRUE(grid.spatialGrid.count(key2));
    EXPECT_EQ(grid.spatialGrid[key2].size(), 1);
    EXPECT_EQ(grid.spatialGrid[key2][0], way1.get());
}

TEST_F(SpatialGridTest, AddMultipleWays) {
    grid.add_way(way1.get());
    grid.add_way(way2.get());
    
    // node1 is shared by both ways
    auto key1 = std::make_pair(10, 20);
    ASSERT_TRUE(grid.spatialGrid.count(key1));
    EXPECT_EQ(grid.spatialGrid[key1].size(), 2);
    
    // node2 is only in way1
    auto key2 = std::make_pair(15, 25);
    ASSERT_TRUE(grid.spatialGrid.count(key2));
    EXPECT_EQ(grid.spatialGrid[key2].size(), 1);
    
    // node3 is only in way2
    auto key3 = std::make_pair(11, 22);
    ASSERT_TRUE(grid.spatialGrid.count(key3));
    EXPECT_EQ(grid.spatialGrid[key3].size(), 1);
}


/*
=======================================
Edge cases
======================================
*/
TEST_F(SpatialGridTest, NegativeCoordinates) {
    auto negativeNode = std::make_unique<Node>();
    negativeNode->coordinates = {-5.5, -10.5};
    
    auto negativeWay = std::make_unique<WayInfo>();
    negativeWay->nodes.push_back(negativeNode.get());
    
    grid.add_way(negativeWay.get());
    
    auto key = std::make_pair(-5, -10); // -5.5/1.0 = -5.5 → static_cast<int> = -5?
    ASSERT_TRUE(grid.spatialGrid.count(key));
    EXPECT_EQ(grid.spatialGrid[key].size(), 1);
}

TEST_F(SpatialGridTest, EmptyWay) {
    auto emptyWay = std::make_unique<WayInfo>();
    
    // Should handle gracefully
    EXPECT_NO_THROW(grid.add_way(emptyWay.get()));
    EXPECT_TRUE(grid.spatialGrid.empty());
}

TEST_F(SpatialGridTest, DuplicateNodes) {
    // Way with duplicate nodes
    auto dupWay = std::make_unique<WayInfo>();
    dupWay->nodes.push_back(node1.get());
    dupWay->nodes.push_back(node1.get()); // Duplicate
    
    grid.add_way(dupWay.get());
    
    // Should only be added once per unique cell
    auto key = std::make_pair(10, 20);
    ASSERT_TRUE(grid.spatialGrid.count(key));
    EXPECT_EQ(grid.spatialGrid[key].size(), 1);
}


/*
=======================================
Cell size
======================================
*/

TEST_F(SpatialGridTest, DifferentCellSizes) {
    SpatialGrid largeGrid;
    largeGrid.cellSize = 10.0; // Larger cells
    
    largeGrid.add_way(way1.get());
    
    // With cellSize=10, both nodes should map to same cell (1,2)
    auto key = std::make_pair(1, 2);
    ASSERT_TRUE(largeGrid.spatialGrid.count(key));
    EXPECT_EQ(largeGrid.spatialGrid[key].size(), 1);
    
    // Original grid with cellSize=1 should have separate cells
    grid.add_way(way1.get());
    EXPECT_GT(grid.spatialGrid.size(), 1);
}

TEST_F(SpatialGridTest, VerySmallCellSize) {
    SpatialGrid tinyGrid;
    tinyGrid.cellSize = 0.001; // 1mm cells
    
    tinyGrid.add_way(way1.get());
    
    // Each node should be in a distinct cell
    EXPECT_EQ(tinyGrid.spatialGrid.size(), 2);
}


/*
=======================================
Performance
======================================
*/
TEST_F(SpatialGridTest, AddManyWaysPerformance) {
    const int NUM_WAYS = 1000;
    const int NODES_PER_WAY = 10;
    
    std::vector<std::unique_ptr<Node>> nodes;
    std::vector<std::unique_ptr<WayInfo>> ways;
    
    // Create a grid of nodes
    for (int i = 0; i < NUM_WAYS * NODES_PER_WAY; i++) {
        auto node = std::make_unique<Node>();
        node->coordinates = {
            static_cast<double>(i % 100), 
            static_cast<double>(i / 100)
        };
        nodes.push_back(std::move(node));
    }
    
    // Create ways connecting nodes
    for (int i = 0; i < NUM_WAYS; i++) {
        auto way = std::make_unique<WayInfo>();
        for (int j = 0; j < NODES_PER_WAY; j++) {
            way->nodes.push_back(nodes[i*NODES_PER_WAY + j].get());
        }
        ways.push_back(std::move(way));
    }
    
    // Time the insertion
    auto start = std::chrono::high_resolution_clock::now();
    for (auto& way : ways) {
        grid.add_way(way.get());
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Added " << NUM_WAYS << " ways in " << duration.count() << " ms" << std::endl;
    
    // Verify all ways were added
    size_t totalEntries = 0;
    for (const auto& entry : grid.spatialGrid) {
        totalEntries += entry.second.size();
    }
    EXPECT_EQ(totalEntries, NUM_WAYS * NODES_PER_WAY);
}


/*
=======================================
get_ways_in_cell
======================================
*/

TEST_F(SpatialGridTest, GetWaysInCellExisting) {
    grid.add_way(way1.get());
    
    // Test for node1's cell (10,20)
    auto result = grid.get_ways_in_cell(10, 20);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().size(), 1);
    EXPECT_EQ(result.value().get()[0], way1.get());
    
    // Test for node2's cell (15,25)
    result = grid.get_ways_in_cell(15, 25);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().size(), 1);
    EXPECT_EQ(result.value().get()[0], way1.get());
}

TEST_F(SpatialGridTest, GetWaysInCellNonExisting) {
    // Empty grid case
    auto result = grid.get_ways_in_cell(0, 0);
    EXPECT_FALSE(result.has_value());
    
    // Non-existent cell in non-empty grid
    grid.add_way(way1.get());
    result = grid.get_ways_in_cell(99, 99);
    EXPECT_FALSE(result.has_value());
}

TEST_F(SpatialGridTest, GetWaysInCellMultipleWays) {
    grid.add_way(way1.get());
    grid.add_way(way2.get());
    
    // Cell (10,20) should have both ways
    auto result = grid.get_ways_in_cell(10, 20);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().size(), 2);
    
    // Verify both ways are present
    bool hasWay1 = false, hasWay2 = false;
    for (auto way : result.value().get()) {
        if (way == way1.get()) hasWay1 = true;
        if (way == way2.get()) hasWay2 = true;
    }
    EXPECT_TRUE(hasWay1);
    EXPECT_TRUE(hasWay2);
}

TEST_F(SpatialGridTest, GetWaysInCellNegativeCoords) {
    auto negativeNode = std::make_unique<Node>();
    negativeNode->coordinates = {-5.5, -10.5};
    
    auto negativeWay = std::make_unique<WayInfo>();
    negativeWay->nodes.push_back(negativeNode.get());
    
    grid.add_way(negativeWay.get());
    
    auto result = grid.get_ways_in_cell(-5, -10);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().size(), 1);
    EXPECT_EQ(result.value().get()[0], negativeWay.get());
}

TEST_F(SpatialGridTest, GetWaysInCellConstCorrectness) {
    grid.add_way(way1.get());
    
    // Test const version
    const auto& constGrid = grid;
    auto result = constGrid.get_ways_in_cell(10, 20);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().size(), 1);
}


/*
=======================================
get_ways_in_cell with coordinates
======================================
*/

TEST_F(SpatialGridTest, GetWaysByCoordinatesExisting) {
    grid.add_way(way1.get());
    
    // Test with node1's exact coordinates
    Coordinates coords{10.0, 20.0};
    auto result = grid.get_ways_in_cell(coords);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get().size(), 1);
    EXPECT_EQ(result->get()[0], way1.get());
}

TEST_F(SpatialGridTest, GetWaysByCoordinatesWithinCell) {
    grid.add_way(way1.get());
    
    // Coordinates that should map to same cell as node1 (10,20)
    Coordinates coords{10.7, 20.3};  // Within (10.0-11.0, 20.0-21.0)
    auto result = grid.get_ways_in_cell(coords);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get().size(), 1);
    EXPECT_EQ(result->get()[0], way1.get());
}

TEST_F(SpatialGridTest, GetWaysByCoordinatesNonExisting) {
    // Empty grid case
    Coordinates coords{0.0, 0.0};
    auto result = grid.get_ways_in_cell(coords);
    EXPECT_FALSE(result.has_value());
    
    // Non-existent coordinates in non-empty grid
    grid.add_way(way1.get());
    coords = {100.0, 100.0};
    result = grid.get_ways_in_cell(coords);
    EXPECT_FALSE(result.has_value());
}

TEST_F(SpatialGridTest, GetWaysByCoordinatesCellBoundary) {
    grid.cellSize = 10.0;  // Larger cells for boundary testing
    
    auto boundaryNode = std::make_unique<Node>();
    boundaryNode->coordinates = {15.0, 25.0};
    
    auto boundaryWay = std::make_unique<WayInfo>();
    boundaryWay->nodes.push_back(boundaryNode.get());
    grid.add_way(boundaryWay.get());
    
    // Test right on cell boundary (15/10=1.5 → cell 1)
    Coordinates coords{15.0, 25.0};
    auto result = grid.get_ways_in_cell(coords);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get().size(), 1);
    EXPECT_EQ(result->get()[0], boundaryWay.get());
}

TEST_F(SpatialGridTest, GetWaysByCoordinatesNegativeValues) {
    grid.cellSize = 5.0;
    
    auto negativeNode = std::make_unique<Node>();
    negativeNode->coordinates = {-12.3, -7.8};
    
    auto negativeWay = std::make_unique<WayInfo>();
    negativeWay->nodes.push_back(negativeNode.get());
    grid.add_way(negativeWay.get());
    
    // Should map to cell (-3,-2)
    Coordinates coords{-12.3, -7.8};
    auto result = grid.get_ways_in_cell(coords);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get().size(), 1);
    EXPECT_EQ(result->get()[0], negativeWay.get());
}

TEST_F(SpatialGridTest, GetWaysByCoordinatesConstCorrectness) {
    grid.add_way(way1.get());
    
    const auto& constGrid = grid;
    Coordinates coords{10.0, 20.0};
    auto result = constGrid.get_ways_in_cell(coords);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get().size(), 1);
    
    // Verify we can't modify through the reference
    static_assert(std::is_const_v<std::remove_reference_t<decltype(result->get())>>, 
                 "Should return const reference");
}

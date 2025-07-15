#include <stdafx.h>

#include <common/spatial/r_tree.h>

using namespace tjs::common;

TEST(RTreeTest, InsertAndQuery) {
	RTree<int> tree;
	BoundingBox b1 { 0, 0, 1, 1 };
	BoundingBox b2 { 2, 2, 3, 3 };
	tree.insert(b1, 1);
	tree.insert(b2, 2);

	std::vector<int> out;
	tree.query(BoundingBox { 0.5, 0.5, 2.5, 2.5 }, std::back_inserter(out));
	EXPECT_EQ(out.size(), 2);
	EXPECT_TRUE(std::find(out.begin(), out.end(), 1) != out.end());
	EXPECT_TRUE(std::find(out.begin(), out.end(), 2) != out.end());
}

TEST(RTreeTest, QueryEmpty) {
	RTree<int> tree;
	BoundingBox b1 { 0, 0, 1, 1 };
	tree.insert(b1, 1);

	std::vector<int> out;
	tree.query(BoundingBox { 2, 2, 3, 3 }, std::back_inserter(out));
	EXPECT_TRUE(out.empty());
}

TEST(RTreeTest, HandlesManyInserts) {
	RTree<int> tree;
	// insert 10 boxes scattered along x-axis
	for (int i = 0; i < 10; i++) {
		BoundingBox b { double(i), 0.0, double(i) + 0.5, 0.5 };
		tree.insert(b, i);
	}
	EXPECT_EQ(tree.size(), 10);
	std::vector<int> out;
	tree.query(BoundingBox { 2.25, 0, 7.25, 1 }, std::back_inserter(out));
	std::sort(out.begin(), out.end());
	ASSERT_EQ(out.size(), 5);
	for (int i = 3; i <= 7; i++) {
		EXPECT_EQ(out[i - 3], i);
	}
}

#include <stdafx.h>

#include <common/container_ptr_handler.h>

using namespace tjs::common;

using IntValueHandler = ContainerPtrHolder<std::vector<int>>;

TEST(ContainerPtrHolderTest, DereferenceAndAccess) {
	std::vector<int> data = { 10, 20, 30 };
	IntValueHandler holder(data, 1); // points to 20

	EXPECT_EQ(*holder, 20);
}

TEST(ContainerPtrHolderTest, MutableAccess) {
	std::vector<int> data = { 1, 2, 3 };
	IntValueHandler holder(data, 2);

	*holder = 42;
	EXPECT_EQ(data[2], 42);
}

TEST(ContainerPtrHolderTest, ConstAccess) {
	std::vector<int> data = { 100, 200, 300 };
	const IntValueHandler holder(data, 0);

	EXPECT_EQ(*holder, 100);
}

TEST(ContainerPtrHolderTest, IndexChange) {
	std::vector<int> data = { 5, 6, 7, 8 };
	IntValueHandler holder(data, 0);

	EXPECT_EQ(*holder, 5);

	holder.set_id(3);
	EXPECT_EQ(*holder, 8);
}

TEST(ContainerPtrHolderTest, ResizeTest) {
	std::vector<int> data = { 5, 6, 7, 8 };
	IntValueHandler holder(data, 2);

	int* current_address = &data[2];

	data.resize(10);

	EXPECT_NE(current_address, &data[2]);
	EXPECT_EQ(*holder, 7);
}

TEST(ContainerPtrHolderTest, StructTest) {
	struct TestStruct {
		int x = 0;
	};

	std::vector<TestStruct> data = { { 0 }, { 6 }, { 7 }, { 8 } };
	const ContainerPtrHolder<std::vector<TestStruct>> holder(data, 2);

	using ValueType = std::vector<TestStruct>::value_type;
	TestStruct xx { 0 };
	const ValueType* x = &xx;

	data.resize(10);

	EXPECT_EQ(holder->x, 7);
}

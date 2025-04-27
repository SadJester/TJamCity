#include "stdafx.h"


TEST(MY_SUITE, FirstTest) {
	ASSERT_EQ(1, 1);
}

int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return ::RUN_ALL_TESTS();
}
#include "stdafx.h"

#include <core/utils/smoothed_value.h>

using namespace tjs;

TEST(SmoothedValueTest, BasicUpdateAndGet) {
	SmoothedValue val { 10.0f, 0.5f };
	val.update(20.0f);
	EXPECT_FLOAT_EQ(val.get_raw(), 20.0f);
	EXPECT_FLOAT_EQ(val.get(), 15.0f); // (10 * 0.5 + 20 * 0.5)
}

TEST(SmoothedValueTest, MultipleUpdates) {
	SmoothedValue val { 0.0f, 0.1f };
	val.update(10.0f); // smoothed = 1.0
	val.update(10.0f); // smoothed = 1.9
	EXPECT_NEAR(val.get(), 1.9f, 1e-5f);
}

TEST(SmoothedValueTest, SetValueResetsBoth) {
	SmoothedValue val { 5.0f, 0.3f };
	val.set(3.0f);
	EXPECT_FLOAT_EQ(val.get_raw(), 3.0f);
	EXPECT_FLOAT_EQ(val.get(), 3.0f);
}

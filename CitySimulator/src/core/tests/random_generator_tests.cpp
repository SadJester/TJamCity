#include <stdafx.h>

#include <core/random_generator.h>

using namespace tjs::core;

class RandomGeneratorTest : public ::testing::Test {
protected:
	void SetUp() override {
		RandomGenerator::reset_seed();
	}
};

TEST_F(RandomGeneratorTest, SingletonAccess) {
	auto& rng1 = RandomGenerator::get();
	auto& rng2 = RandomGenerator::get();
	EXPECT_EQ(&rng1, &rng2);
}

TEST_F(RandomGeneratorTest, BooleanGeneration) {
	auto& rng = RandomGenerator::get();

	// Test probability bounds
	EXPECT_NO_THROW(rng.next_bool(0.0));
	EXPECT_NO_THROW(rng.next_bool(1.0));
	EXPECT_THROW(rng.next_bool(1.1), std::invalid_argument);
	EXPECT_THROW(rng.next_bool(-0.1), std::invalid_argument);

	// Test edge cases
	EXPECT_FALSE(rng.next_bool(0.0));
	EXPECT_TRUE(rng.next_bool(1.0));
}

TEST_F(RandomGeneratorTest, IntegerGeneration) {
	auto& rng = RandomGenerator::get();

	// Test range
	for (int i = 0; i < 100; ++i) {
		int val = rng.next_int(1, 10);
		EXPECT_GE(val, 1);
		EXPECT_LE(val, 10);
	}

	// Test single value
	EXPECT_EQ(rng.next_int(5, 5), 5);

	// Test invalid range
	EXPECT_THROW(rng.next_int(10, 1), std::invalid_argument);
}

TEST_F(RandomGeneratorTest, FloatGeneration) {
	auto& rng = RandomGenerator::get();

	// Test default range
	for (int i = 0; i < 100; ++i) {
		float val = rng.next_float();
		EXPECT_GE(val, 0.0f);
		EXPECT_LT(val, 1.0f);
	}

	// Test custom range
	for (int i = 0; i < 100; ++i) {
		float val = rng.next_float(2.5f, 3.5f);
		EXPECT_GE(val, 2.5f);
		EXPECT_LT(val, 3.5f);
	}

	// Test invalid range
	EXPECT_THROW(rng.next_float(3.5f, 2.5f), std::invalid_argument);
}

TEST_F(RandomGeneratorTest, DoubleGeneration) {
	auto& rng = RandomGenerator::get();

	// Test default range
	for (int i = 0; i < 100; ++i) {
		double val = rng.next_double();
		EXPECT_GE(val, 0.0);
		EXPECT_LT(val, 1.0);
	}

	// Test custom range
	for (int i = 0; i < 100; ++i) {
		double val = rng.next_double(-1.0, 1.0);
		EXPECT_GE(val, -1.0);
		EXPECT_LT(val, 1.0);
	}

	// Test invalid range
	EXPECT_THROW(rng.next_double(1.0, -1.0), std::invalid_argument);
}

TEST_F(RandomGeneratorTest, SeedControl) {
	// Set fixed seed
	RandomGenerator::set_seed(42);
	auto& rng = RandomGenerator::get();

	int val1 = rng.next_int(1, 1000);
	int val2 = rng.next_int(1, 1000);

	// Reset same seed should produce same sequence
	RandomGenerator::set_seed(42);
	int val3 = rng.next_int(1, 1000);
	int val4 = rng.next_int(1, 1000);

	EXPECT_EQ(val1, val3);
	EXPECT_EQ(val2, val4);

	// Reset to random seeding
	RandomGenerator::reset_seed();
	int val5 = rng.next_int(1, 1000);
	EXPECT_NE(val1, val5); // Very high probability of being different
}

TEST_F(RandomGeneratorTest, ThreadSafety) {
	constexpr int num_threads = 4;
	constexpr int samples_per_thread = 1000;
	std::vector<std::thread> threads;
	std::vector<std::unordered_set<int>> results(num_threads);

	for (int i = 0; i < num_threads; ++i) {
		threads.emplace_back([i, &results]() {
			auto& rng = RandomGenerator::get();
			for (int j = 0; j < samples_per_thread; ++j) {
				results[i].insert(rng.next_int(1, 1000000));
			}
		});
	}

	for (auto& t : threads) {
		t.join();
	}
	// If we get here without crashing, test passes
	SUCCEED();
}

TEST_F(RandomGeneratorTest, DistributionQuality) {
	auto& rng = RandomGenerator::get();
	constexpr int samples = 10000;

	// Test boolean distribution
	int true_count = 0;
	for (int i = 0; i < samples; ++i) {
		if (rng.next_bool(0.3)) {
			true_count++;
		}
	}
	double ratio = static_cast<double>(true_count) / samples;
	EXPECT_GT(ratio, 0.28);
	EXPECT_LT(ratio, 0.32);

	// Test integer distribution uniformity
	constexpr int buckets = 10;
	std::vector<int> counts(buckets, 0);
	for (int i = 0; i < samples; ++i) {
		int val = rng.next_int(0, buckets - 1);
		counts[val]++;
	}

	// Check all buckets have reasonable counts
	int min_count = *std::min_element(counts.begin(), counts.end());
	int max_count = *std::max_element(counts.begin(), counts.end());

	EXPECT_GT(min_count, samples / (buckets * 2)); // At least half of expected
	EXPECT_LT(max_count, samples * 2 / buckets);   // At most double expected
}

TEST(RandomGeneratorEnumTest, BasicEnumGeneration) {
	enum class TestEnum {
		One,
		Two,
		Three,
		Count // Must be last
	};

	auto& rng = RandomGenerator::get();

	// Test with enum that has Count
	std::unordered_set<TestEnum> test_values;
	for (int i = 0; i < 100; ++i) {
		test_values.insert(rng.next_enum<TestEnum>());
	}

	EXPECT_TRUE(test_values.contains(TestEnum::One));
	EXPECT_TRUE(test_values.contains(TestEnum::Two));
	EXPECT_TRUE(test_values.contains(TestEnum::Three));
	EXPECT_EQ(3, test_values.size()) << "Should not generate values outside enum range";
}

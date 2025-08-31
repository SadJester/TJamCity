#include <gtest/gtest.h>
#include <benchmark/benchmark.h>

#if defined(GTEST_BENCHMARK_MAIN)
GTEST_BENCHMARK_MAIN();
#else
int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	benchmark::Initialize(&argc, argv);
	const int test_result = RUN_ALL_TESTS();
	if (test_result != 0) {
		return test_result;
	}
	benchmark::RunSpecifiedBenchmarks();
	return 0;
}
#endif

#include <stdafx.h>
#include <benchmark/benchmark.h>

TEST(BenchmarkExample, sanity_check) {
	EXPECT_EQ(1, 1);
}

static void noop_benchmark(benchmark::State& state) noexcept {
	for (auto _ : state) {
		benchmark::DoNotOptimize(_);
	}
}
BENCHMARK(noop_benchmark);

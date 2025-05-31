#pragma once

namespace tjs::core {
	class RandomGenerator {
	public:
		RandomGenerator(const RandomGenerator&) = delete;
		RandomGenerator& operator=(const RandomGenerator&) = delete;
		static RandomGenerator& get();

		// Set global seed for all threads (version >= 0)
		static void set_seed(uint64_t seed);
		// Reset to per-thread robust seeding (version = -1)
		static void reset_seed();

		// Generate bool with probability p for true (default: 0.5)
		bool next_bool(float p = 0.5f);

		template<typename T>
		T next(T min, T max);
		int next_int(int min, int max);
		float next_float();
		float next_float(float min, float max);
		double next_double();
		double next_double(double min, double max);

	private:
		RandomGenerator();
		void check_reseed();

	private:
		// Static atomic variables for global seed control
		static std::atomic<uint64_t> global_seed;
		static std::atomic<int> seed_version;

		int _local_seed_version;
		std::mt19937_64 _engine;
	};
} // namespace tjs::core

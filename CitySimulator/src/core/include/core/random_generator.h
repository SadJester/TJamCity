#pragma once

namespace tjs::core {
	class RandomGenerator {
	public:
		// TODO: Get generator for different purposes
		template<typename Purpose>
		class Controller {};

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

		// Enum support
		template<typename Enum>
			requires(
				std::is_enum_v<Enum>
				&& requires(Enum t) {
					   { static_cast<int>(Enum::Count) } -> std::same_as<int>;
				   })
		Enum next_enum() {
			constexpr auto count = static_cast<std::underlying_type_t<Enum>>(Enum::Count);
			return static_cast<Enum>(next_int(0, count - 1));
		}

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
		// sonar-ignore-start:S2245 - in this place we treat that this is not sensitive context
		std::mt19937_64 _engine;
		// sonar-ignore-end:S2245
	};
} // namespace tjs::core

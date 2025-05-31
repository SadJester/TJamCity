#include <core/stdafx.h>

#include <core/random_generator.h>

namespace tjs::core {
	// Initialize static members
	std::atomic<uint64_t> RandomGenerator::global_seed(0);
	std::atomic<int> RandomGenerator::seed_version(-1);

	RandomGenerator& RandomGenerator::get() {
		static thread_local RandomGenerator generator;
		generator.check_reseed();
		return generator;
	}

	RandomGenerator::RandomGenerator()
		: _local_seed_version(-2) {
		check_reseed(); // Initialize with proper seeding
	}

	void RandomGenerator::set_seed(uint64_t seed) {
		global_seed.store(seed, std::memory_order_relaxed);
		seed_version.fetch_add(1, std::memory_order_release);
	}

	void RandomGenerator::reset_seed() {
		seed_version.store(-1, std::memory_order_release);
	}

	void RandomGenerator::check_reseed() {
		const int current_version = seed_version.load(std::memory_order_acquire);

		if (_local_seed_version == current_version) {
			return;
		}

		if (current_version >= 0) {
			// Seed from global seed
			_engine.seed(global_seed.load(std::memory_order_relaxed));
		} else {
			// Robust per-thread seeding
			std::random_device rd;
			const auto time_seed = static_cast<uint64_t>(
				std::chrono::high_resolution_clock::now()
					.time_since_epoch()
					.count());
			const auto thread_seed = static_cast<uint64_t>(
				std::hash<std::thread::id> {}(std::this_thread::get_id()));

			std::seed_seq seed_seq {
				rd(), rd(), // From random_device
				static_cast<uint32_t>(time_seed),
				static_cast<uint32_t>(time_seed >> 32),
				static_cast<uint32_t>(thread_seed),
				static_cast<uint32_t>(thread_seed >> 32)
			};
			_engine.seed(seed_seq);
		}
		_local_seed_version = current_version;
	}

	bool RandomGenerator::next_bool(float p) {
		if (p < 0.0f || p > 1.0f) {
			// TODO: algo error handling
			throw std::invalid_argument("Probability must be in [0.0, 1.0]");
		}
		std::bernoulli_distribution dist(p);
		return dist(_engine);
	}

	template<typename T>
	T RandomGenerator::next(T min, T max) {
		static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");
		check_reseed();
		if (min > max) {
			throw std::invalid_argument("min must be <= max");
		}

		if constexpr (std::is_integral_v<T>) {
			std::uniform_int_distribution<T> dist(min, max);
			return dist(_engine);
		} else {
			std::uniform_real_distribution<T> dist(min, max);
			return dist(_engine);
		}
	}

	template int RandomGenerator::next<int>(int, int);
	template float RandomGenerator::next<float>(float, float);
	template double RandomGenerator::next<double>(double, double);

	int RandomGenerator::next_int(int min, int max) {
		return next<int>(min, max);
	}

	float RandomGenerator::next_float() {
		return next(0.0f, 1.0f);
	}

	float RandomGenerator::next_float(float min, float max) {
		return next<float>(min, max);
	}

	double RandomGenerator::next_double() {
		return next(0.0, 1.0);
	}

	double RandomGenerator::next_double(double min, double max) {
		return next<double>(min, max);
	}

} // namespace tjs::core

#pragma once

namespace tjs::core {
	namespace simulation {
		class TrafficSimulationSystem;
	} // namespace simulation

	using SimClock = std::chrono::system_clock;
	using SimDuration = std::chrono::duration<double>; // allows fractional seconds
	using SimTimePoint = std::chrono::time_point<SimClock, SimDuration>;

	struct TimeState {
		double simulationTime = 0.0;
		double unscaledsimulationTime = 0.0f;

		double timeDelta = 0.0f;
		double unscaledTimeDelta = 0.0f;

		double timeMultiplier = 1.0;
		bool isPaused = false;
		bool stepRequested = false;

		void set_fixed_delta(double value) {
			fixed_delta = value;
		}
		double fixed_dt() const {
			return fixed_delta;
		}

		void init_start_time(SimTimePoint&& start) {
			sim_time_start = start;
			current_sim_time = sim_time_start;
		}

		const SimTimePoint& start_time() const {
			return sim_time_start;
		}
		const SimTimePoint& current_time() const {
			return current_sim_time;
		}

		void tick() {
			const auto dt_duration = SimDuration(fixed_delta);
			current_sim_time += dt_duration;
		}

	private:
		double fixed_delta = 0.0;

		SimTimePoint sim_time_start;
		SimTimePoint current_sim_time;
	};

	class TimeModule {
	public:
		static constexpr double DEFAULT_STEP_TIME = 0.016;
		// 5 km with 60 km/h will take 5 min
		// to make it pass on the screen for 10 seconds time should be scaled x30
		static constexpr double DEFAULT_TIME_MULTIPLIER = 30;

	public:
		TimeModule(simulation::TrafficSimulationSystem& system, double stepDelta = TimeModule::DEFAULT_STEP_TIME);

		void initialize();
		void update(double realTimeDelta);

		void speed_up();
		void slow_down();
		void set_time_multiplier(double value);
		void set_step_delta(double value);

		const TimeState& state() const;

		void pause();
		void resume();
		void tick();

	private:
		simulation::TrafficSimulationSystem& _system;
		TimeState _timeState;
		double _stepDelta = 0.016;
	};
} // namespace tjs::core

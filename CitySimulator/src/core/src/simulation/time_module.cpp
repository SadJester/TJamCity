#include <core/stdafx.h>

#include <core/simulation/time_module.h>
#include <core/simulation/simulation_system.h>

namespace tjs::core {

	TimeModule::TimeModule(simulation::TrafficSimulationSystem& system, double stepDelta)
		: _system(system)
		, _stepDelta(stepDelta) {
	}

	void TimeModule::initialize() {
		_timeState.timeMultiplier = TimeModule::DEFAULT_TIME_MULTIPLIER;
		_timeState.init_start_time(SimClock::from_time_t(std::mktime(new std::tm {
			0, 0, 6,   // sec, min, hour
			13, 6, 125 // day, month (0-based), years since 1900 => 2025-07-13
		})));
		_timeState.set_fixed_delta(_system.settings().step_delta_sec);
	}

	void TimeModule::update(double realTimeDelta) {
		if (!_timeState.isPaused || _timeState.stepRequested) {
			_timeState.unscaledTimeDelta = realTimeDelta;
			_timeState.timeDelta = realTimeDelta * _timeState.timeMultiplier;

			_timeState.simulationTime += _timeState.timeDelta;
			_timeState.unscaledsimulationTime += realTimeDelta;
		}
	}

	void TimeModule::speed_up() {
		_timeState.timeMultiplier *= 1.1;
	}

	void TimeModule::slow_down() {
		_timeState.timeMultiplier /= 1.1;
	}

	void TimeModule::set_time_multiplier(double value) {
		if (value <= 0.0) {
			// TODO: algo error handling
			throw std::invalid_argument("Time multiplier must be positive");
		}
		_timeState.timeMultiplier = value;
	}

	void TimeModule::set_step_delta(double value) {
		if (value <= 0.0) {
			// TODO: algo error handling
			throw std::invalid_argument("Step delta must be positive");
		}
		_stepDelta = value;
		_timeState.set_fixed_delta(value);
	}

	const TimeState& TimeModule::state() const {
		return _timeState;
	}

	void TimeModule::pause() {
		_timeState.isPaused = true;
		_timeState.timeDelta = 0;
		_timeState.unscaledTimeDelta = 0;
	}

	void TimeModule::resume() {
		_timeState.isPaused = false;
	}
	void TimeModule::tick() {
		_timeState.stepRequested = true;
		update(_stepDelta);
		_timeState.tick();
		_timeState.stepRequested = false;
	}

} // namespace tjs::core

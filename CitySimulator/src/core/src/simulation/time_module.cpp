#include <core/stdafx.h>

#include <core/simulation/time_module.h>

namespace tjs::core {

	TimeModule::TimeModule(double stepDelta)
		: _stepDelta(stepDelta) {
		_timeState.timeMultiplier = TimeModule::DEFAULT_TIME_MULTIPLIER;
	}

	void TimeModule::update(double realTimeDelta) {
		if (!_timeState.isPaused || _timeState.stepRequested) {
			_timeState.unscaledTimeDelta = realTimeDelta;
			_timeState.timeDelta = realTimeDelta * _timeState.timeMultiplier;

			_timeState.simulationTime += _timeState.timeDelta;
			_timeState.unscaledsimulationTime += realTimeDelta;

			_timeState.stepRequested = false;
		}
	}

	void TimeModule::speed_up() {
		_timeState.timeMultiplier *= 1.1;
	}

	void TimeModule::slow_down() {
		_timeState.timeMultiplier /= 1.1;
	}

	void TimeModule::set_time_multiplier(double value) {
		_timeState.timeMultiplier = value;
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
	void TimeModule::step() {
		_timeState.stepRequested = true;
		update(_stepDelta);
	}

} // namespace tjs::core

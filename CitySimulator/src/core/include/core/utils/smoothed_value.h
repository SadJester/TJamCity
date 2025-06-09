#pragma once


namespace tjs {
    struct SmoothedValue {
		SmoothedValue(float initial, float smoothing_factor)
			: _value(initial)
			, _smoothing_factor(smoothing_factor)
			, _smoothed_value(initial){
			}

		void update(float value) {
			_value = value;
			_smoothed_value = _smoothed_value * (1.0 - _smoothing_factor) + _value * _smoothing_factor;
		}

		float get()const {
			return _smoothed_value;
		}

		float get_raw() const {
			return _value;
		}

		void set(float value) {
			_smoothed_value = value;
			_value = value;
		}

	private:
		float _smoothed_value = 60.0;
		float _value = 0.f;
		float _smoothing_factor;
	};
}

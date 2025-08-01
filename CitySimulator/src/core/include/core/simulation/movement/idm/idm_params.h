#pragma once

namespace tjs::core::simulation::idm {
	struct idm_params_t {
		float s0 = 2.0f;         // minimum jam distance [m]
		float t_headway = 1.5f;  // safe time headway [s]
		float a_max = 1.0f;      // maximum acceleration [m/s²]
		float b_comf = 2.0f;     // comfortable deceleration [m/s²] (positive)
		float b_hard = 7.5f;     // max deceleration [m/s²] (positive)
		float v_desired = 30.0f; // desired cruise speed [m/s]
		float delta = 4.0f;      // acceleration exponent (4 = standard IDM)
	};
} // namespace tjs::core::simulation::idm

#pragma once

namespace tjs::core::simulation::idm {
	struct idm_params_t {
		float s0 = 2.0f;                // minimum jam distance [m]
		float t_headway = 0.8f;         // safe time headway [s] in the city
		float t_headway_highway = 1.5f; // safe time headway [s] on highways
		float a_max = 1.5f;             // maximum acceleration [m/s²]
		float b_comf = 2.0f;            // comfortable deceleration [m/s²] (positive)
		float b_hard = 7.5f;            // max deceleration [m/s²] (positive)
		float v_desired = 30.0f;        // desired cruise speed [m/s]
		float v_limits_violate = 0.0f;  // speed [m/s] that can be violated by vehicle
		float delta = 4.0f;             // acceleration exponent (4 = standard IDM)

		bool is_cooperating = true;     // if this vehicle is cooperative one
		float a_coop_max = 1.5f * 0.5f; // maximum accel/break for cooperation
	};
} // namespace tjs::core::simulation::idm

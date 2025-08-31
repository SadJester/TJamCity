#pragma once

namespace tjs::core::simulation::idm {
	struct idm_params_t {
		double t_cooldown = 1.5f;       // [s] cool‑down expiry threshold
		double t_prepare = 0.1f;        // [s] time preparing for crossing (perception of a driver)
		double t_cross = 2.0f;          // [s] what time to cross tha lanes
		double t_align = 0.1f;          // [s] time to align to the lane center
		double t_max_coop_time = 10.0f; // [s] time for cooperating

		float t_headway = 0.8f;             // [s] safe time headway in the city
		float t_headway_highway = 1.5f;     // [s] safe time headway on highways
		float t_cross_headway_coeff = 0.5f; // coeff applied to t_headway* while lane changing

		float s0 = 2.0f;               // [m] minimum jam distance
		float a_max = 1.5f;            // [m/s²] maximum acceleration
		float b_comf = 2.0f;           // [m/s²] comfortable deceleration (positive)
		float b_hard = 7.5f;           // [m/s²] max deceleration (positive)
		float v_desired = 30.0f;       // [m/s] desired cruise speed
		float v_limits_violate = 0.0f; // [m/s] speed that can be violated by vehicle
		float delta = 4.0f;            // acceleration exponent (4 = standard IDM)

		float cooperation_probability = 1.0f; // probability that vehicle will be cooperative
		float a_coop_max = 1.5f * 0.5f;       // [m/s²] maximum accel/break for cooperation

		float s_preparation = 80.0f;         // [m] distance to start lane‑prep
		float a_politeness_threshold = 2.5f; // [m/s²] threshold that should be for !mandatory lane change

		bool is_cooperating = true; // if this vehicle is cooperative one
	};
} // namespace tjs::core::simulation::idm

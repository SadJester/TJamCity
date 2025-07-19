#pragma once

namespace tjs::core::simulation {

	ENUM(VehicleStateBits, uint8_t,
		None = 0,
		FOLLOW = 1,
		PREPARE_LC = 2,
		EXECUTE_LC = 3,
		YIELD_LC = 4);

	struct VehicleBuffers {
		std::vector<double> s_curr;          // position at start of tick
		std::vector<float> v_curr;           // speed at start of tick
		std::vector<float> v_next;           // write-only inside Phase A
		std::vector<double> s_next;          // (optional) longitudinal advance
		std::vector<float> desired_v;        // constant
		std::vector<float> length;           // constant
		std::vector<float> lateral_off;      // 0 = centred
		std::vector<Lane*> lane;             // index into lane table
		std::vector<Lane*> lane_target;      // ≡ nullptr when FOLLOW
		std::vector<VehicleStateBits> flags; // packed bits
	};

} // namespace tjs::core::simulation

#pragma once

namespace tjs::core::simulation {

	ENUM_FLAG(VehicleStateBits, uint8_t,
		None = 0);

	struct VehicleBuffers {
		std::vector<double> s_curr;          // position at start of tick
		std::vector<float> v_curr;           // speed at start of tick
		std::vector<float> v_next;           // write-only inside Phase A
		std::vector<double> s_next;          // (optional) longitudinal advance
		std::vector<float> desired_v;        // constant
		std::vector<float> length;           // constant
		std::vector<float> lateral_off;      // 0 = centred
		std::vector<uint32_t> lane_id;       // index into lane table
		std::vector<VehicleStateBits> flags; // packed bits
	};

} // namespace tjs::core::simulation

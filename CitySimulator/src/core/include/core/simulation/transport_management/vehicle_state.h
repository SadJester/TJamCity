#pragma once

namespace tjs::core {
	struct Lane;
	struct Vehicle;
} // namespace tjs::core

namespace tjs::core::simulation {

	enum VehicleStateBits : uint8_t {
		ST_FOLLOW = 0b00, // lower 2 bits = finite-state machine
		ST_PREPARE = 0b01,
		ST_EXECUTE = 0b10,
		ST_YIELD = 0b11,
		FL_COOLDOWN = 0b100, // bit-2  : “cool-down running”
		FL_ERROR = 0b1000,   // bit-3  : any movement error
	};

	inline void set_state(uint8_t& f, uint8_t st) noexcept {
		f = uint8_t((f & ~0b11) | (st & 0b11));
	}

	inline uint8_t get_state(uint8_t f) noexcept {
		return f & 0b11;
	}

	struct VehicleBuffers {
		std::vector<double> s_curr;     // position at start of tick
		std::vector<float> v_curr;      // speed at start of tick
		std::vector<float> v_next;      // write-only inside Phase A
		std::vector<double> s_next;     // (optional) longitudinal advance
		std::vector<float> desired_v;   // constant
		std::vector<float> length;      // constant
		std::vector<float> lateral_off; // 0 = centred
		std::vector<Lane*> lane;        // index into lane table
		std::vector<Lane*> lane_target; // ≡ nullptr when FOLLOW
		std::vector<float> v_max_speed; // time from last lane change
		std::vector<uint8_t> flags;     // packed bits

		std::vector<size_t> uids;

		void clear();
		void add_vehicle(Vehicle& vehicle);
		void reserve(size_t count);
	};

} // namespace tjs::core::simulation

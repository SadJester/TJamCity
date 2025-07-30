#pragma once

#include <common/bits_enum.h>

namespace tjs::core {
	struct Lane;
	struct Vehicle;
} // namespace tjs::core

namespace tjs::core::simulation {

	// ----------------------------
	// Division: State & Flags (in one 16-bit field)
	// ----------------------------

	enum class VehicleStateBitsDivision : uint16_t {
		STATE = 0x00FF, // bits 0–7 (room for 8 FSM states + 8 flags)
		FLAGS = 0xFF00  // bits 8–15
	};

	// ----------------------------
	// State + Flag values
	// ----------------------------

	enum class VehicleStateBits : uint16_t {
		// STATE bits (bits 0–7)
		ST_FOLLOW = 1 << 0,
		ST_PREPARE = 1 << 1,
		ST_EXECUTE = 1 << 2,
		ST_YIELD = 1 << 3,
		ST_STOPPED = 1 << 4,
		ST_WAIT = 1 << 5,
		ST_RESERVED1 = 1 << 6,
		ST_RESERVED2 = 1 << 7,

		// FLAGS (bits 8–15)
		FL_COOLDOWN = 1 << 8,
		FL_ERROR = 1 << 9,
		FL_RESERVED2 = 1 << 10,
		FL_RESERVED3 = 1 << 11,
		FL_RESERVED4 = 1 << 12,
		FL_RESERVED5 = 1 << 13,
		FL_RESERVED6 = 1 << 14,
		FL_RESERVED7 = 1 << 15
	};

	enum class VehicleMErrorsDivision : uint16_t {
		ERROR = 0xFFFF // full 16 bits for independent error flags
	};

	enum class VehicleMovementErrors : uint16_t {
		ER_NO_ERROR = 1 << 0,
		ER_NO_OUTGOING_CONNECTION = 1 << 1,
		ER_NO_PATH = 1 << 2,
		ER_INCORRECT_EDGE = 1 << 3,
		ER_INCORRECT_LANE = 1 << 4,
		ER_NO_NEXT_LANE = 1 << 5,
		ER_BLOCKED = 1 << 6,
		ER_REROUTE_FAILED = 1 << 7,
		ER_NO_TARGET = 1 << 8,
		ER_COLLISION = 1 << 9,
		ER_UNKNOWN = 1 << 10
	};

	using VehicleStateBitsV = common::BitMaskValue<VehicleStateBits, VehicleStateBitsDivision>;
	using MovementErrors = common::BitMaskValue<VehicleMovementErrors, VehicleMErrorsDivision>;

	/*
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
	}*/

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
		std::vector<uint16_t> flags;    // packed bits

		std::vector<size_t> uids;

		void clear();
		void add_vehicle(Vehicle& vehicle);
		void reserve(size_t count);
	};

} // namespace tjs::core::simulation

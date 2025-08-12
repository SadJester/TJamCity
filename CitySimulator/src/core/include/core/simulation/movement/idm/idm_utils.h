#pragma once

#include <core/simulation/movement/idm/idm_params.h>
#include <core/simulation/movement/movement_runtime_structures.h>

namespace tjs::core {
	struct Vehicle;
} // namespace tjs::core

namespace tjs::core::simulation::idm {

	float safe_entry_speed(const float v_leader, const float gap, const double dt) noexcept;
	// Physical bumper‑to‑bumper gap between follower and its leader.
	// Negative/zero gaps are clamped to 0 to avoid division by zero downstream.
	float actual_gap(const float s_leader_ctr, const float s_follower_ctr, const float len_leader, const float len_follower) noexcept;

	// Desired dynamic gap s* (IDM eq. 3) that keeps time‑headway and braking distance.
	// delta_v = v_follower - v_leader.  Positive when closing in.
	float desired_gap(const float v_follower, const float delta_v, const idm_params_t& p) noexcept;

	bool gap_ok(const LaneRuntime& tgt_rt,
		const std::vector<Vehicle>& vehicles,
		const double s_new, // tentative bumper pos
		const float len_new,
		const idm::idm_params_t& p,
		const double dt,
		const std::size_t row_newcomer);

} // namespace tjs::core::simulation::idm

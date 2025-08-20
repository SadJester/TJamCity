#include <core/stdafx.h>

#include <core/simulation/movement/idm/idm_utils.h>
#include <core/data_layer/vehicle.h>

namespace tjs::core::simulation::idm {
	float safe_entry_speed(const float v_leader, const float gap, const double dt) noexcept {
		// Prevent division by zero when dt≈0
		if (dt <= 1e-6) {
			return v_leader;
		}
		// Allow the follower to close the entire gap within the next tick
		return v_leader + gap / static_cast<float>(dt);
	}

	float actual_gap(const float s_leader_ctr,
		const float s_follower_ctr,
		const float len_leader,
		const float len_follower) noexcept {
		const float bumper_dist =
			(s_leader_ctr - s_follower_ctr) - 0.5f * (len_leader + len_follower);
		return std::max(0.0f, bumper_dist);
	}

	float desired_gap(const float v_follower,
		const float delta_v,
		const idm_params_t& p) noexcept {
		const float braking_term = (v_follower * delta_v) / (2.0f * std::sqrt(p.a_max * p.b_comf));
		const float dyn = v_follower * p.t_headway + braking_term;
		return p.s0 + std::max(0.0f, dyn);
	}

	bool gap_ok(const LaneRuntime& tgt_rt,
		const Vehicle& vehicle_newcomer,
		const double s_new, // tentative bumper pos
		const float len_new,
		const idm::idm_params_t& p,
		const double dt) {
		const auto& idx = tgt_rt.idx; // descending s_curr

		// Find insertion point (same as before)
		auto it = std::lower_bound(idx.begin(), idx.end(), s_new,
			[&](Vehicle* vehicle, double pos) {
				return vehicle->s_on_lane > pos;
			});

		auto enough_gap_and_brake = [&](float gap,
										float v_follow,
										float v_lead) -> bool {
			if (gap < p.s0) {
				return false; // hard minimum jam distance
			}

			float delta_v = v_follow - v_lead; // +ve when closing
			if (delta_v <= 0.0f) {
				return true; // diverging – fine
			}

			// (i) Brake needed to **stop** before leader using constant decel
			float req_brake = (delta_v * delta_v) / (2.0f * std::max(1e-3f, gap - p.s0));
			if (req_brake > p.b_hard + 1e-4f) {
				return false;
			}

			// (ii) Per‑tick decel limit (so we don't exceed −b_comf in this step)
			float per_tick = delta_v / static_cast<float>(dt);
			if (per_tick > p.b_hard + 1e-4f) {
				// return false;
			}

			return true;
		};

		/* ---------- leader gap ------------------------------------------------- */
		if (it != idx.begin()) {
			Vehicle* j_lead = *(it - 1);
			float gap = idm::actual_gap(static_cast<float>(j_lead->s_on_lane),
				static_cast<float>(s_new),
				j_lead->length, len_new);
			if (!enough_gap_and_brake(gap, /* follower = newcomer */
					vehicle_newcomer.currentSpeed, j_lead->currentSpeed)) {
				return false;
			}
		}

		/* ---------- follower (vehicle behind newcomer) ------------------------- */
		if (it != idx.end()) {
			Vehicle* j_follow = *it;
			float gap = idm::actual_gap(static_cast<float>(s_new),
				static_cast<float>(j_follow->s_on_lane),
				len_new, j_follow->length);
			if (!enough_gap_and_brake(gap, /* follower behind */
					j_follow->currentSpeed, vehicle_newcomer.currentSpeed)) {
				return false;
			}
		}

		return true;
	}

} // namespace tjs::core::simulation::idm

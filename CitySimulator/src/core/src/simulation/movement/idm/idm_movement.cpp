#include <core/stdafx.h>

#include <core/simulation/movement/idm/idm_movement.h>

#include <core/simulation/simulation_system.h>

#include <core/simulation/movement/idm/lane_agnostic_movement.h>

namespace tjs::core::simulation {

	static core::Coordinates lane_position(const Lane& lane,
		double s,             // longitudinal [m]
		double lateral_offset // lateral [m]
	) {
		if (lane.centerLine.empty()) {
			return {};
		}

		const auto& start = lane.centerLine.front();
		const auto& end = lane.centerLine.back();

		/* ---------- longitudinal interpolation on the centre-line --------- */
		const double len = std::max(lane.length, 1e-6);    // avoid div-by-zero
		const double frac = std::clamp(s / len, 0.0, 1.0); // clamp within lane

		core::Coordinates pos;
		pos.x = start.x + frac * (end.x - start.x);
		pos.y = start.y + frac * (end.y - start.y);

		/* ---------- lateral shift ----------------------------------------- */
		if (std::abs(lateral_offset) < 1e-6) {
			return pos; // nothing to do
		}

		// unit direction vector along the lane
		double dx = end.x - start.x;
		double dy = end.y - start.y;
		const double inv = 1.0 / std::hypot(dx, dy);
		dx *= inv;
		dy *= inv;

		// left-hand normal vector = (-dy, +dx)
		const double nx = -dy;
		const double ny = dx;

		pos.x += lateral_offset * nx;
		pos.y += lateral_offset * ny;
		return pos;
	}


	IDMMovementAlgo::IDMMovementAlgo(TrafficSimulationSystem& system)
		: IMovementAlgorithm(MovementAlgoType::IDM, system) {
	}

	void IDMMovementAlgo::update() {
		auto& vs = _system.vehicle_system();
		auto& vehicles = vs.vehicles();
		auto& lane_rt = vs.lane_runtime();

		double dt = _system.timeModule().state().fixed_dt();

		idm::phase1_simd(_system, vehicles, lane_rt, dt);
		idm::phase2_commit(_system, vehicles, lane_rt, dt);

		for (size_t i = 0; i < vehicles.size(); ++i) {
			Vehicle& v = vehicles[i];
			if (v.has_position_changes && v.current_lane) {
				v.has_position_changes = false;
				v.coordinates = lane_position(*v.current_lane, v.s_on_lane, v.lateral_offset);
				v.rotationAngle = v.current_lane->rotation_angle;
			}
		}
	}

} // namespace tjs::core::simulation

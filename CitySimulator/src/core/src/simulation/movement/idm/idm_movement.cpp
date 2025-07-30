#include <core/stdafx.h>

#include <core/simulation/movement/idm/idm_movement.h>

#include <core/simulation/simulation_system.h>

#include <core/simulation/movement/lane_agnostic_movement.h>

namespace tjs::core::simulation {

	IDMMovementAlgo::IDMMovementAlgo(TrafficSimulationSystem& system)
		: IMovementAlgorithm(MovementAlgoType::IDM, system) {
	}

	void IDMMovementAlgo::update() {
		auto& vs = _system.vehicle_system();
		auto& buf = vs.vehicle_buffers();
		auto& lane_rt = vs.lane_runtime();

		double dt = _system.timeModule().state().fixed_dt();

		phase1_simd(_system, buf, lane_rt, dt);
		phase2_commit(_system, buf, lane_rt, dt);
		_system.vehicle_system().commit();
	}

} // namespace tjs::core::simulation

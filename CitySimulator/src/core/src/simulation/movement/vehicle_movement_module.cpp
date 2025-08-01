#include <core/stdafx.h>

#include <core/simulation/movement/vehicle_movement_module.h>
#include <core/simulation/simulation_system.h>

#include <core/simulation/movement/idm/idm_movement.h>
#include <core/simulation/movement/agent/agent_movement.h>

namespace tjs::core::simulation {
	VehicleMovementModule::VehicleMovementModule(TrafficSimulationSystem& system)
		: _system(system) {
	}

	void VehicleMovementModule::initialize() {
		switch (_system.settings().movement_algo) {
			case MovementAlgoType::Agent:
				_algorithm = std::make_unique<AgentMovementAlgo>(_system);
				break;
			case MovementAlgoType::IDM:
			default:
				_algorithm = std::make_unique<IDMMovementAlgo>(_system);
				break;
		}

		_algorithm->initialize();
	}

	void VehicleMovementModule::release() {
		_algorithm->release();
		_algorithm.reset();
	}

	void VehicleMovementModule::update() {
		TJS_TRACY_NAMED("VehicleMovement_Update");

		_algorithm->update();
	}

} // namespace tjs::core::simulation

#include <core/stdafx.h>

#include <core/simulation/tactical/tactical_planning_module.h>
#include <core/simulation/simulation_system.h>

#include <core/data_layer/data_types.h>

namespace tjs::simulation {

	TacticalPlanningModule::TacticalPlanningModule(TrafficSimulationSystem& system)
		: _system(system) {
	}

	void TacticalPlanningModule::update() {
		auto& agents = _system.agents();
		for (size_t i = 0; i < agents.size(); ++i) {
			updateAgentTactics(agents[i]);
		}
	}

	void TacticalPlanningModule::updateAgentTactics(tjs::simulation::AgentData& agent) {
		if (agent.vehicle == nullptr) {
			return;
		}

		using namespace tjs::core;

		auto& vehicle = *agent.vehicle;

		if (vehicle.currentWay == nullptr || vehicle.currentSegmentIndex + 1 >= vehicle.currentWay->nodes.size()) {
			return;
		}

		Node* currentNode = vehicle.currentWay->nodes[vehicle.currentSegmentIndex];
		Node* nextNode = vehicle.currentWay->nodes[vehicle.currentSegmentIndex + 1];
		Coordinates dir {
			nextNode->coordinates.latitude - currentNode->coordinates.latitude,
			nextNode->coordinates.longitude - currentNode->coordinates.longitude
		};
		vehicle.rotationAngle = atan2(dir.longitude, dir.latitude);
	}

} // namespace tjs::simulation

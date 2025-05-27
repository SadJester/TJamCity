#pragma once
#include "core/data_layer/data_types.h"
#include "core/simulation/agent/agent_data.h"

namespace tjs::simulation {
	class TrafficSimulationSystem;
	using namespace tjs::core;

	class TacticalPlanningModule {
	public:
		TacticalPlanningModule(TrafficSimulationSystem& system);

		void update();

	private:
		void updateAgentTactics(tjs::simulation::AgentData& agent);
		int findClosestSegmentIndex(const Coordinates& coords, WayInfo* way);
		double distanceToSegment(const Coordinates& point,
			const Coordinates& segStart,
			const Coordinates& segEnd);
		Node* findNearestNode(const Coordinates& coords, RoadNetwork& road_network);

	private:
		TrafficSimulationSystem& _system;
	};
} // namespace tjs::simulation

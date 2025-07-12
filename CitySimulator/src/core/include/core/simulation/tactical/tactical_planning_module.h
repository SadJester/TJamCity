#pragma once
#include "core/data_layer/data_types.h"
#include "core/simulation/agent/agent_data.h"

namespace tjs::core::simulation {
	class TrafficSimulationSystem;

	class TacticalPlanningModule {
	public:
		TacticalPlanningModule(TrafficSimulationSystem& system);

		void initialize();
		void release();
		void update();

	private:
		void update_agent_tactics(core::AgentData& agent);
		int find_closest_segmen_index(const Coordinates& coords, WayInfo* way);
		double distance_to_segment(
			const Coordinates& point,
			const Coordinates& segStart,
			const Coordinates& segEnd);

	private:
		TrafficSimulationSystem& _system;
	};

	namespace simulation_details {
		void update_agent(AgentData& agent, TrafficSimulationSystem& system);
	} // namespace simulation_details

} // namespace tjs::core::simulation

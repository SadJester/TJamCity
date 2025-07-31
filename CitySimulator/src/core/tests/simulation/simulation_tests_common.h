#pragma once

#include <core/data_layer/world_data.h>
#include <core/store_models/idata_model.h>
#include <core/simulation/simulation_system.h>
#include <data_loader_mixin.h>

namespace tjs::core::tests {
	class SimulationTestsCommon
		: public ::testing::Test,
		  public DataLoaderMixin {
	public:
		virtual std::string default_map() const {
			return "test_lanes.osmx";
		}
		void SetUp() override;

		virtual bool prepare();
		virtual bool load_map();
		virtual void set_up_settings();

		void create_basic_system();

		WorldSegment& get_segment() {
			return *world.segments().front();
		}

		Node* get_node(uint64_t id) {
			return world.segments()[0]->nodes[id].get();
		}

		WayInfo* get_way(uint64_t id) {
			return world.segments()[0]->ways[id].get();
		}

		RoadNetwork& get_road_network() {
			return *world.segments()[0]->road_network;
		}

	protected:
		std::string _default_map;
		tjs::core::WorldData world;
		tjs::core::model::DataModelStore store;
		std::unique_ptr<tjs::core::simulation::TrafficSimulationSystem> system;
		tjs::core::SimulationSettings settings;
	};
} // namespace tjs::core::tests

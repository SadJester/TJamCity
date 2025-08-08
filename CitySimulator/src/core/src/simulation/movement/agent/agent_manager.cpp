#include <core/stdafx.h>

#include <core/simulation/agent/agent_manager.h>
#include <core/simulation/simulation_system.h>
#include <core/simulation/transport_management/transport_generator.h>

#include <core/simulation/transport_management/vehicle_system.h>
#include <core/random_generator.h>
#include <core/events/vehicle_population_events.h>

#include <core/store_models/vehicle_analyze_data.h>
#include <core/data_layer/world_data.h>
#include <core/data_layer/way_info.h>

namespace tjs::core::simulation {

	namespace details {

		void sync_agents(Agents& agents, Vehicles& vehicles) {
			for (size_t i = 0; i < agents.size(); ++i) {
				agents[i].vehicle = &vehicles[i];
			}
		}

		// Generate till count that was set
		class BulkGenerator : public ITransportGenerator {
		public:
			BulkGenerator(VehicleSystem& vehicle_system, TrafficSimulationSystem& system)
				: ITransportGenerator(system)
				, _vehicles(vehicle_system) {
			}

			void start_populating() override {
				_expected_vehicles = system().settings().vehiclesCount;
				_creation_ticks = 0;
				_state = State::InProgress;
			}

			size_t populate() override {
				if (is_done() || system().worldData().segments().empty()) {
					return 0;
				}

				++_creation_ticks;

				const auto& configs = system().vehicle_system().vehicle_configs();
				auto& lane_rt = system().vehicle_system().lane_runtime();
				size_t created = 0;
				auto& segment = system().worldData().segments()[0];

				auto& agents = system().agent_manager().agents();
				auto& vehicles = system().vehicle_system().vehicles();

				const size_t max_attempts = 100;
				size_t attempts = 0;
				auto& edges = segment->road_network->edges;

				// TODO: more prettier code. Don`t skip on review!
				auto& vehicles_info = _vehicles.vehicles();
				while (vehicles_info.size() < _expected_vehicles && attempts < max_attempts) {
					auto& edge = edges[RandomGenerator::get().next_int(0, edges.size() - 1)];
					Lane* lane = &edge.lanes[0];

					auto type = RandomGenerator::get().next_enum<VehicleType>();
					auto result = _vehicles.create_vehicle(*lane, type);
					if (!result.has_value()) {
						continue;
					}
					agents.push_back({ vehicles[result.value()].uid, &vehicles[result.value()] });
					++created;
				}

				if (_creation_ticks > 1000 && _state != State::Completed) {
					_state = State::Error;
				}

				if (_vehicles.vehicles().size() >= _expected_vehicles) {
					_state = State::Completed;
				}

				// TEMPORARY
				sync_agents(agents, vehicles);

				return created;
			}

			bool is_done() const override {
				return _state == State::Completed || _state == State::Error;
			}

		private:
			VehicleSystem& _vehicles;
			size_t _expected_vehicles = 0;
			size_t _creation_ticks = 0;
		};

		struct VehicleSpawnRequest {
			Lane* lane;
			Node* goal;
			double vehicles_per_hour;
			double accumulator = 0.0;

			VehicleSpawnRequest(Lane* lane_, double vh_per_hour, Node* goal_)
				: lane(lane_)
				, goal(goal_)
				, vehicles_per_hour(vh_per_hour) {
			}
		};

		class FlowVehicleGenerator : public ITransportGenerator {
		public:
			FlowVehicleGenerator(TrafficSimulationSystem& system)
				: ITransportGenerator(system)
				, _vehicle_system(system.vehicle_system()) {
			}

			void start_populating() override {
				_state = State::InProgress;

				for (auto& p : _spawn_requests) {
					p.accumulator = 0.0;
				}

				if (system().worldData().segments().empty()) {
					return;
				}

				_spawn_requests.clear();

				auto& segment = system().worldData().segments().front();
				auto& edges = segment->road_network->edges;

				auto& settings = system().settings();
				for (const auto& req : settings.spawn_requests) {
					Lane* lane = nullptr;
					for (auto& edge : edges) {
						for (auto& l : edge.lanes) {
							if (l.get_id() == req.lane_id) {
								lane = &l;
								break;
							}
						}
						if (lane) {
							break;
						}
					}

					Node* goal = nullptr;
					auto it = segment->nodes.find(req.goal_node_id);
					if (it != segment->nodes.end()) {
						goal = it->second.get();
					}

					if (lane) {
						_spawn_requests.emplace_back(
							lane,
							static_cast<double>(req.vehicles_per_hour),
							goal);
					}
				}
			}

			size_t populate() override {
				if (is_done() || system().worldData().segments().empty()) {
					return 0;
				}

				size_t created = 0;

				const auto& configs = system().vehicle_system().vehicle_configs();
				auto& lane_rt = system().vehicle_system().lane_runtime();
				auto& agents = system().agent_manager().agents();
				auto& vehicles = _vehicle_system.vehicles();

				const double dt = _system.timeModule().state().fixed_dt();
				for (auto& point : _spawn_requests) {
					point.accumulator += point.vehicles_per_hour * (dt / 3600.0); // dt in seconds
					while (point.accumulator >= 1.0) {
						auto type = RandomGenerator::get().next_enum<VehicleType>();
						auto result = _vehicle_system.create_vehicle(*point.lane, type);
						if (result.has_value()) {
							agents.push_back({ vehicles[result.value()].uid, &vehicles[result.value()] });
							++created;
						} else {
							break; // no space
						}
					}
				}

				// TEMPORARY
				sync_agents(agents, vehicles);

				return created;
			}

			bool is_done() const override {
				return _state == State::Completed || _state == State::Error;
			}

		private:
			VehicleSystem& _vehicle_system;
			std::vector<VehicleSpawnRequest> _spawn_requests;
		};
	} // namespace details

	AgentManager::AgentManager(TrafficSimulationSystem& system)
		: _system(system) {
	}

	AgentManager::~AgentManager() {
	}

	void AgentManager::initialize() {
		auto& settings = _system.settings();

		_agents.clear();
		_agents.reserve(settings.vehiclesCount);
		_creation_ticks = 0;

		switch (_system.settings().generator_type) {
			case simulation::GeneratorType::Bulk:
				_generator = std::make_unique<details::BulkGenerator>(_system.vehicle_system(), _system);
				break;
			case simulation::GeneratorType::Flow:
			default:
				_generator = std::make_unique<details::FlowVehicleGenerator>(_system);
				break;
		}

		if (_system.worldData().segments().empty()) {
			return;
		}

		_generator->start_populating();
	}

	void AgentManager::release() {
	}

	void AgentManager::update() {
		if (_generator->is_done()) {
			return;
		}
		++_creation_ticks;

		size_t created = _generator->populate();
		bool need_send = created != 0;
		const bool has_errors = _generator->get_state() == ITransportGenerator::State::Error;
		if (_generator->is_done()) {
			need_send = true;

			_system.vehicle_system().set_state(has_errors ? VehicleSystem::CreationState::Error : VehicleSystem::CreationState::Completed);
			if (_agents.size() == 1) {
				auto& store = _system.store();
				store.get_entry<core::model::VehicleAnalyzeData>()->agent = &_agents[0];
			}
		}

		if (need_send) {
			_system.message_dispatcher().handle_message(
				core::events::VehiclesPopulated {
					created,
					_agents.size(),
					_system.settings().vehiclesCount,
					_creation_ticks,
					has_errors },
				"vehicle_system");
		}
	}

} // namespace tjs::core::simulation

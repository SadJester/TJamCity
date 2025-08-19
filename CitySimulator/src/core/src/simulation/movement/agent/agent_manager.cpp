#include <core/stdafx.h>

#include <core/simulation/agent/agent_manager.h>
#include <core/simulation/simulation_system.h>
#include <core/simulation/agent/agent_generator.h>

#include <core/simulation/transport_management/vehicle_system.h>
#include <core/random_generator.h>
#include <core/events/vehicle_population_events.h>

#include <core/store_models/vehicle_analyze_data.h>
#include <core/data_layer/world_data.h>
#include <core/data_layer/way_info.h>

namespace tjs::core::simulation {

	namespace details {

		void sync_agents(std::vector<AgentPtr>& agents, Vehicles& vehicles) {
			for (size_t i = 0; i < agents.size(); ++i) {
				if (agents[i] && i < vehicles.size()) {
					agents[i]->vehicle = &vehicles[i];
				}
			}
		}

		// Generate till count that was set
		class BulkGenerator : public IAgentGenerator {
		public:
			BulkGenerator(
				VehicleSystem& vehicle_system,
				TrafficSimulationSystem& system,
				AgentPool& agent_pool,
				std::vector<AgentData*>& _agents,
				std::unordered_map<AgentData*, AgentPtr>& _agent_handles
			)
				: IAgentGenerator(system)
				, _vehicles(vehicle_system)
				, _agent_pool(agent_pool)
				, _agents(_agents)
				, _agent_handles(_agent_handles) {
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

				auto& agent_manager = system().agent_manager();

				const size_t max_attempts = 100;
				size_t attempts = 0;
				auto& edges = segment->road_network->edges;

				auto& vehicles_info = _vehicles.vehicles();
				while (vehicles_info.size() < _expected_vehicles && attempts < max_attempts) {
					++attempts;
					auto& edge = edges[RandomGenerator::get().next_int(0, edges.size() - 1)];
					Lane* lane = &edge.lanes[RandomGenerator::get().next_int(0, edge.lanes.size() - 1)];

					auto type = RandomGenerator::get().next_enum<VehicleType>();
					auto result = _vehicles.create_vehicle(*lane, type);
					if (!result.has_value()) {
						continue;
					}
					
					// Create agent using object pool
					auto agent_ptr = _agent_pool.acquire(result.value()->uid, result.value());
					
					// Add to agents vector and handles map (like vehicle system)
					_agents.push_back(agent_ptr.get());
					_agent_handles[agent_ptr.get()] = agent_ptr;
					
					result.value()->agent = agent_ptr.get();
					
					++created;
				}

				if (_creation_ticks > 1000 && _state != State::Completed) {
					// TODO[simulation]: log error no space
					_state = State::Error;
				}

				if (_vehicles.vehicles().size() >= _expected_vehicles) {
					// TODO[simulation]: log completed
					_state = State::Completed;
				}
				return created;
			}

			bool is_done() const override {
				return _state == State::Completed || _state == State::Error;
			}

		private:
			VehicleSystem& _vehicles;
			AgentPool& _agent_pool;
			std::vector<AgentData*>& _agents;
			std::unordered_map<AgentData*, AgentPtr>& _agent_handles;
			size_t _expected_vehicles = 0;
			size_t _creation_ticks = 0;
		};

		struct VehicleSpawnRequest {
			Lane* lane;
			Node* goal;
			AgentGoalSelectionType goal_selection_type;
			double vehicles_per_hour;
			int max_vehicles;
			double accumulator = 0.0;
			int generated = 0;

			VehicleSpawnRequest(Lane* lane_, double vh_per_hour, Node* goal_, int max, AgentGoalSelectionType goal_type_)
				: lane(lane_)
				, goal(goal_)
				, vehicles_per_hour(vh_per_hour)
				, max_vehicles(max)
				, goal_selection_type(goal_type_) {
			}
		};

		class FlowVehicleGenerator : public IAgentGenerator {
		public:
			FlowVehicleGenerator(
				TrafficSimulationSystem& system,
				AgentPool& agent_pool,
				std::vector<AgentData*>& _agents,
				std::unordered_map<AgentData*, AgentPtr>& _agent_handles
			)
				: IAgentGenerator(system)
				, _vehicle_system(system.vehicle_system())
				, _agent_pool(agent_pool)
				, _agents(_agents)
				, _agent_handles(_agent_handles) {
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
					if (req.goal_selection_type == AgentGoalSelectionType::GoalNodeId) {
						auto it = segment->nodes.find(req.goal_node_id);
						if (it != segment->nodes.end()) {
							goal = it->second.get();
						}
					}

					if (lane) {
						_spawn_requests.emplace_back(
							lane,
							static_cast<double>(req.vehicles_per_hour),
							goal,
							req.max_vehicles,
							req.goal_selection_type);
					}
				}
			}

			size_t populate() override {
				if (is_done() || system().worldData().segments().empty()) {
					return 0;
				}

				size_t created = 0;

				const auto& configs = _vehicle_system.vehicle_configs();
				auto& lane_rt = system().vehicle_system().lane_runtime();

				const double dt = _system.timeModule().state().fixed_dt();
				for (auto& point : _spawn_requests) {
					if (point.max_vehicles > 0 && point.generated >= point.max_vehicles) {
						continue;
					}
					point.accumulator += point.vehicles_per_hour * (dt / 3600.0); // dt in seconds
					while (point.accumulator >= 1.0
						   && (point.max_vehicles == 0 || point.generated < point.max_vehicles)) {
						auto type = RandomGenerator::get().next_enum<VehicleType>();
						auto result = _vehicle_system.create_vehicle(*point.lane, type);
						if (result.has_value()) {
							// Create agent using object pool
							auto agent_ptr = _agent_pool.acquire(result.value()->uid, result.value());
							agent_ptr->profile.goal_selection = point.goal_selection_type;
							agent_ptr->profile.goal = point.goal;

							// Add to agents vector and handles map (like vehicle system)
							_agents.push_back(agent_ptr.get());
							_agent_handles[agent_ptr.get()] = agent_ptr;
							
							result.value()->agent = agent_ptr.get();
							
							++created;
							++point.generated;
							point.accumulator = 0.0;
						} else {
							break; // no space
						}
					}
				}

				return created;
			}

			bool is_done() const override {
				return _state == State::Completed || _state == State::Error;
			}

		private:
			VehicleSystem& _vehicle_system;
			AgentPool& _agent_pool;
			std::vector<AgentData*>& _agents;
			std::unordered_map<AgentData*, AgentPtr>& _agent_handles;
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

		_creation_ticks = 0;

		// Clear existing agents and handles
		_agent_handles.clear();
		_agents.clear();
		
		// Reserve capacity in the object pool
		_agent_pool.destroy_all_live();
		_agent_pool.reserve(settings.vehiclesCount);

		switch (_system.settings().generator_type) {
			case simulation::GeneratorType::Bulk:
				_generator = std::make_unique<details::BulkGenerator>(_system.vehicle_system(), _system, _agent_pool, _agents, _agent_handles);
				break;
			case simulation::GeneratorType::Flow:
			default:
				_generator = std::make_unique<details::FlowVehicleGenerator>(_system, _agent_pool, _agents, _agent_handles);
				break;
		}

		if (_system.worldData().segments().empty()) {
			return;
		}

		_generator->start_populating();
	}

	void AgentManager::release() {
		// ObjectPool will automatically clean up when destroyed
	}

	void AgentManager::update() {
		remove_agents();
		populate_agents();
	}

	void AgentManager::populate_agents() {
		if (_generator->is_done()) {
			return;
		}
		++_creation_ticks;

		size_t created = _generator->populate();
		bool need_send = created != 0;
		const bool has_errors = _generator->get_state() == IAgentGenerator::State::Error;
		if (_generator->is_done()) {
			need_send = true;
			if (_agents.size() == 1) {
				auto& store = _system.store();
				store.get_entry<core::model::VehicleAnalyzeData>()->agent = _agents[0];
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

	void AgentManager::remove_agents() {
		// Remove agents marked for removal from both active_agents and agents/handles
		/*size_t write_index = 0;
		for (size_t read_index = 0; read_index < _agents.size(); ++read_index) {
			if (_agents[read_index] && _agents[read_index]->to_remove) {
				// Get the agent pointer for removal
				AgentData* agent_ptr = _agents[read_index].get();
				
				// Remove from agents vector
				auto it = std::find(_agents.begin(), _agents.end(), agent_ptr);
				if (it != _agents.end()) {
					_agents.erase(it);
				}
				
				// Remove from handles map and release back to pool
				auto handle_it = _agent_handles.find(agent_ptr);
				if (handle_it != _agent_handles.end()) {
					_agent_pool.release(handle_it->second);
					_agent_handles.erase(handle_it);
				}
				
				// Mark as invalid in active_agents
				_agents[read_index] = AgentPtr{};
			} else {
				// Keep this agent and update its index
				if (write_index != read_index) {
					_agents[write_index] = std::move(_active_agents[read_index]);
				}
				if (_active_agents[write_index] && _active_agents[write_index]->vehicle) {
					_active_agents[write_index]->vehicle->agent_idx = write_index;
				}
				++write_index;
			}
		}*/
	}

	void AgentManager::remove_agent(AgentData& agent) {
		agent.stucked = true;
		agent.to_remove = true;
	}

} // namespace tjs::core::simulation

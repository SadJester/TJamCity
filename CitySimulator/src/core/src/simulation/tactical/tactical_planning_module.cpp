#include <core/stdafx.h>

#include <core/simulation/tactical/tactical_planning_module.h>
#include <core/simulation/simulation_system.h>
#include <core/simulation/movement/movement_utils.h>

#include <core/data_layer/data_types.h>
#include <core/data_layer/world_data.h>
#include <core/map_math/earth_math.h>

#include <core/map_math/path_finder.h>

#include <latch>

namespace tjs::core::simulation {

	TacticalPlanningModule::TacticalPlanningModule(TrafficSimulationSystem& system)
		: _system(system) {
	}

	TacticalPlanningModule::~TacticalPlanningModule() {
		_pool.reset();
	}

	void TacticalPlanningModule::initialize() {
		_pool.reset();
		_pool = std::make_unique<_test::ThreadPool>(std::thread::hardware_concurrency() - 3);
	}

	void TacticalPlanningModule::release() {
	}

	namespace _test {
		// --- very small pool ---
		class ThreadPool {
		public:
			explicit ThreadPool(unsigned n = std::thread::hardware_concurrency())
				: stop(false) {
				workers.reserve(n ? n : 1);
				_n = n;
				for (unsigned t = 0; t < (n ? n : 1); ++t) {
					workers.emplace_back([this, t] {
						tracy::SetThreadName(("upd/" + std::to_string(t)).c_str());
						for (;;) {
							std::function<void()> job;
							{
								std::unique_lock lk(mx);
								cv.wait(lk, [&] { return stop || !q.empty(); });
								if (stop && q.empty()) {
									return;
								}
								job = std::move(q.front());
								q.pop();
							}
							ZoneScopedN("job");
							job();
						}
					});
				}
			}
			~ThreadPool() {
				{
					std::scoped_lock lk(mx);
					stop = true;
				}
				cv.notify_all();
				for (auto& w : workers) {
					w.join();
				}
			}
			template<class F>
			void enqueue(F&& f) {
				{
					std::scoped_lock lk(mx);
					q.emplace(std::forward<F>(f));
				}
				cv.notify_one();
			}
			void wait_idle() {
				std::latch done(workers.size());

				for (unsigned i = 0; i < workers.size(); ++i) {
					enqueue([&done] {
						done.count_down();
					});
				}

				done.wait(); // blocks until all tasks called count_down()
			}
			int _n = 1;

		private:
			std::vector<std::thread> workers;
			std::queue<std::function<void()>> q;
			std::mutex mx;
			std::condition_variable cv;
			bool stop;
		};
	} // namespace _test

	void TacticalPlanningModule::update() {
		TJS_TRACY_NAMED("TacticalPlanning_Update");
		auto& agents = _system.agents();

		const size_t batch_size = agents.size() / _pool->_n;

#if 1
		for (size_t i = 0; i < batch_size; ++i) {
			_pool->enqueue([this, batch_size, i, &agents] {
				size_t m = std::min(agents.size(), i * batch_size + batch_size);
				for (size_t j = i * batch_size; j < m; ++j) {
					simulation_details::update_agent(j, agents[j], _system);
				}
			});
		}
		_pool->wait_idle();
#endif

#if 0
	for (size_t i = 0; i < agents.size(); ++i) {
		simulation_details::update_agent(i, agents[i], _system);	
	}
#endif

#if 0

		for (std::size_t i = 0; i < agents.size(); ++i) {
			_pool->enqueue([this, i, &agents]{
				ZoneScopedN("update_agent");
				simulation_details::update_agent(i, agents[i], _system);
			});
		}
		_pool->wait_idle();
#endif
	}

	namespace simulation_details {
		std::vector<Edge*> find_path(Lane* start_lane, Node* goal, RoadNetwork& road_network, bool look_adjacent_lanes) {
			std::vector<Edge*> result;
			auto edge_path = core::algo::PathFinder::find_edge_path_a_star_from_lane(
				road_network,
				start_lane,
				goal,
				look_adjacent_lanes);
			result.reserve(edge_path.size());
			for (const auto* edge : edge_path) {
				result.push_back(const_cast<Edge*>(edge));
			}

			return result;
		}

		Node* find_nearest_node(const Coordinates& coords, RoadNetwork& road_network) {
			Node* nearest = nullptr;
			double min_distance = std::numeric_limits<double>::max();

			for (const auto& [id, node] : road_network.nodes) {
				double dist = core::algo::euclidean_distance(node->coordinates, coords);
				if (dist < min_distance) {
					min_distance = dist;
					nearest = node;
				}
			}

			return nearest;
		}

		void reset_goals(AgentData& agent, bool success, bool mark = true) {
			agent.currentGoal = nullptr;
			agent.path.clear();
			if (!success) {
				agent.goalFailCount++;
				if (agent.goalFailCount >= 5) {
					agent.stucked = true;
				}
			} else {
				agent.goalFailCount = 0;
				agent.stucked = false;
			}
		}

		void update_agent(size_t i, AgentData& agent, TrafficSimulationSystem& system) {
			TJS_TRACY_NAMED("TacticalPlanning::update_agent");
			if (agent.vehicle == nullptr || agent.currentGoal == nullptr) {
				return;
			}

			auto& vehicle = *agent.vehicle;

			auto& world = system.worldData();
			auto& segment = world.segments().front();
			auto& road_network = *segment->road_network;
			auto& spatial_grid = segment->spatialGrid;
			auto& buf = system.vehicle_system().vehicle_buffers();

			if (VehicleStateBitsV::has_info(buf.flags[i], VehicleStateBits::ST_STOPPED)) {
				// reach goal
				if (vehicle.error == VehicleMovementError::ER_NO_PATH) {
					vehicle.error = VehicleMovementError::ER_NO_ERROR;
					if (agent.currentGoal != nullptr) {
						const double distance_to_target = core::algo::euclidean_distance(vehicle.coordinates, agent.currentGoal->coordinates);
						if (distance_to_target > SimulationConstants::ARRIVAL_THRESHOLD) {
							// TODO[simulation]: handle agent not close enough to target
							reset_goals(agent, true);
						}
					}
					reset_goals(agent, true);
					return;
				}

				if (vehicle.error == VehicleMovementError::ER_NO_OUTGOING_CONNECTION) {
					reset_goals(agent, true);
					agent.stucked = true;
					return;
				}

				if (vehicle.error == VehicleMovementError::ER_INCORRECT_EDGE || vehicle.error == VehicleMovementError::ER_INCORRECT_LANE) {
					// need rebuild path
					agent.path.clear();
					VehicleStateBitsV::remove_info(buf.flags[i], VehicleStateBits::FL_ERROR, VehicleStateBitsDivision::FLAGS);
				}
			}

			// new goal
			if (agent.path.empty() && VehicleStateBitsV::has_info(buf.flags[i], VehicleStateBits::ST_STOPPED)) {
				Node* start_node = vehicle.current_lane->parent->start_node;

				Lane* start_lane = vehicle.current_lane;

				Node* goal_node = agent.currentGoal;
				if (start_lane && goal_node) {
					const bool find_adjacent = vehicle.s_on_lane < (vehicle.current_lane->length - 2.0);
					agent.path = find_path(start_lane, goal_node, road_network, find_adjacent);

					if (!agent.path.empty()) {
						Edge* first_edge = agent.path.front();
						agent.path.insert(agent.path.begin(), start_lane->parent);

						agent.path_offset = 0;
						agent.goal_lane_mask = build_goal_mask(*start_lane->parent, *first_edge);

						agent.distanceTraveled = 0.0; // Reset distance for new path
						agent.goalFailCount = 0;
						VehicleStateBitsV::overwrite_info(buf.flags[i], VehicleStateBits::ST_FOLLOW, VehicleStateBitsDivision::STATE);
						VehicleStateBitsV::remove_info(buf.flags[i], VehicleStateBits::FL_ERROR, VehicleStateBitsDivision::FLAGS);
						vehicle.state = buf.flags[i];

					} else {
						VehicleStateBitsV::set_info(buf.flags[i], VehicleStateBits::FL_ERROR, VehicleStateBitsDivision::FLAGS);
						vehicle.state = buf.flags[i];
						reset_goals(agent, false);
					}
				}
			}
		}

	} // namespace simulation_details

} // namespace tjs::core::simulation

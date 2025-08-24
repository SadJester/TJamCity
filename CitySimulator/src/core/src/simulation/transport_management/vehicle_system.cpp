#include <core/stdafx.h>

#include <core/simulation/transport_management/vehicle_system.h>

#include <core/simulation/simulation_system.h>
#include <core/random_generator.h>
#include <core/events/vehicle_population_events.h>

#include <core/simulation/agent/agent_generator.h>

#include <core/simulation/time_module.h>

#include <core/simulation/movement/idm/idm_utils.h>

//TODO[simulation]: Probably must move from here while moving further
#include <core/data_layer/data_types.h>
#include <core/data_layer/world_data.h>
#include <core/data_layer/way_info.h>
#include <core/data_layer/lane.h>
#include <core/data_layer/lane_vehicle_utils.h>

namespace tjs::core::simulation {

	// Helper function to create vehicle with ObjectPool
	Vehicle* create_vehicle_impl(
		VehicleSystem::VehiclePool& vehicle_pool,
		Lane& lane,
		std::vector<LaneRuntime>& lane_rt,
		const VehicleConfig& config,
		VehicleType type,
		float desired_speed) {
		auto vehicle_ptr = vehicle_pool.acquire_ptr();
		if (!vehicle_ptr) {
			return nullptr;
		}

		Vehicle& vehicle = *vehicle_ptr;
		// TODO[simulation]: correct UID
		vehicle.uid = RandomGenerator::get().next_int(1, 10000000);
		vehicle.type = type;

		vehicle.length = config.length;
		vehicle.width = config.width;
		vehicle.currentSpeed = desired_speed;
		vehicle.maxSpeed = RandomGenerator::get().next_float(40, 100.0f);
		vehicle.coordinates = lane.parent->start_node->coordinates;
		vehicle.currentSegmentIndex = 0;
		vehicle.current_lane = &lane;
		vehicle.s_on_lane = vehicle.length / 2.0f;
		vehicle.lateral_offset = 0.0;
		VehicleStateBitsV::set_info(vehicle.state, VehicleStateBits::ST_STOPPED, VehicleStateBitsDivision::STATE);
		vehicle.previous_state = vehicle.state;
		vehicle.error = VehicleMovementError::ER_NO_ERROR;

		vehicle.s_next = 0.0;
		vehicle.v_next = 0.0f;
		vehicle.lane_target = nullptr;
		vehicle.lane_change_time = 0.0f;
		vehicle.lane_change_dir = 0;

		// we know that this is the last vehicle in the lane (allow_on_lane)
		vehicle.current_lane->vehicles.push_back(&vehicle);
		lane_rt[lane.index_in_buffer].idx.push_back(&vehicle);

		return vehicle_ptr;
	}

	bool allowed_on_lane(const LaneRuntime& lane, float v_length, float v_speed, float dt) {
		if (lane.idx.empty()) {
			return true;
		}

		// 2 meters from bumper
		Vehicle* leader = lane.idx.back();
		return idm::gap_ok(lane, v_speed, v_length / 2.0f, v_length, {}, dt);
	}

	VehicleSystem::VehicleSystem(TrafficSimulationSystem& system)
		: _system(system) {
	}

	VehicleSystem::~VehicleSystem() {
	}

	void VehicleSystem::initialize() {
		if (_system.worldData().segments().empty()) {
			return;
		}

		_vehicle_configs = {
			{ VehicleType::SimpleCar, { VehicleType::SimpleCar, 4.5f, 1.8f } },
			{ VehicleType::SmallTruck, { VehicleType::SmallTruck, 6.5f, 2.5f } },
			{ VehicleType::BigTruck, { VehicleType::BigTruck, 10.0f, 2.5f } },
			{ VehicleType::Ambulance, { VehicleType::Ambulance, 5.5f, 2.2f } },
			{ VehicleType::PoliceCar, { VehicleType::PoliceCar, 5.0f, 2.0f } },
			{ VehicleType::FireTrack, { VehicleType::FireTrack, 8.0f, 2.5f } }
		};

		auto& segment = _system.worldData().segments()[0];
		auto& network = *segment->road_network;

		_lane_runtime.clear();
		for (auto& edge : network.edges) {
			for (auto& lane : edge.lanes) {
				_lane_runtime.push_back({ &lane,
					lane.length,
					edge.way->maxSpeed / 3.6f,
					{} });
				lane.index_in_buffer = _lane_runtime.size() - 1;
			}
		}

		// Reserve capacity in the object pool
		_vehicle_pool.clear();
		_vehicle_pool.reserve(_system.settings().vehiclesCount);
	}

	void VehicleSystem::release() {
		// ObjectPool will automatically clean up when destroyed
	}

	void VehicleSystem::commit() {
		for (LaneRuntime& rt : _lane_runtime) {
			Lane& lane = *rt.static_lane;
			auto& idx = rt.idx;
			lane.vehicles.resize(idx.size());
			for (std::size_t i = 0; i < idx.size(); ++i) {
				lane.vehicles[i] = idx[i];
			}
		}
	}

	std::optional<Vehicle*> VehicleSystem::create_vehicle(Lane& lane, VehicleType type, float desired_speed) {
		auto it_config = _vehicle_configs.find(type);
		if (it_config == _vehicle_configs.end()) {
			// TODO[simulation]: log Vehicle type configuration not found
			it_config = _vehicle_configs.begin();
		}

		const auto& config = it_config->second;
		const auto& lr = _lane_runtime[lane.index_in_buffer];
		double dt = _system.timeModule().state().fixed_dt();
		if (!allowed_on_lane(lr, config.length, desired_speed, dt)) {
			// TODO[simulation]: log no allowed on lane
			return {};
		}
		return create_vehicle_impl(_vehicle_pool, lane, _lane_runtime, config, type, desired_speed);
	}

	void VehicleSystem::update() {
		_vehicle_pool.update_objects();
	}

	void VehicleSystem::remove_vehicle(Vehicle* vehicle) {
		if (!vehicle) {
			return;
		}

		// Remove from lane
		if (vehicle->current_lane) {
			core::remove_vehicle(*vehicle->current_lane, vehicle);

			// Remove from lane runtime
			if (vehicle->current_lane->index_in_buffer < _lane_runtime.size()) {
				auto& idx = _lane_runtime[vehicle->current_lane->index_in_buffer].idx;
				auto it = std::find(idx.begin(), idx.end(), vehicle);
				if (it != idx.end()) {
					idx.erase(it);
				}
			}
		}

		// Release back to pool
		_vehicle_pool.release(vehicle);
	}

} // namespace tjs::core::simulation

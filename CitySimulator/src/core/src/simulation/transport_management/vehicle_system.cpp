#include <core/stdafx.h>

#include <core/simulation/transport_management/vehicle_system.h>

#include <core/simulation/simulation_system.h>
#include <core/random_generator.h>
#include <core/events/vehicle_population_events.h>

//TODO[simulation]: Probably must move from here while moving further
#include <core/data_layer/data_types.h>
#include <core/data_layer/world_data.h>
#include <core/data_layer/way_info.h>
#include <core/data_layer/lane.h>
#include <core/data_layer/lane_vehicle_utils.h>

namespace tjs::core::simulation {

	VehicleSystem::VehicleSystem(TrafficSimulationSystem& system)
		: _system(system) {
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

		_creation_state = CreationState::InProgress;
		_creation_ticks = 0;
		_buffers.clear();
		_vehicles.clear();
		_vehicles.reserve(_system.settings().vehiclesCount);
		_buffers.reserve(_system.settings().vehiclesCount);
	}

	void VehicleSystem::release() {
	}

	static core::Coordinates lane_position(const Lane& lane, double s) {
		if (lane.centerLine.empty()) {
			return {};
		}
		const auto& start = lane.centerLine.front();
		const auto& end = lane.centerLine.back();
		if (s <= 0.0) {
			return start;
		}
		if (lane.length <= 1e-6) {
			return end;
		}
		double fraction = s / lane.length;
		Coordinates result {};
		result.x = start.x + fraction * (end.x - start.x);
		result.y = start.y + fraction * (end.y - start.y);
		return result;
	}

	static float lane_rotation(const Lane& lane) {
		if (lane.centerLine.empty()) {
			return 0.0f;
		}
		const auto& start = lane.centerLine.front();
		const auto& end = lane.centerLine.back();
		return static_cast<float>(atan2(end.y - start.y, end.x - start.x));
	}

	void VehicleSystem::commit() {
		for (size_t i = 0; i < _vehicles.size(); ++i) {
			Vehicle& v = _vehicles[i];
			v.current_lane = _buffers.lane[i];
			v.currentSpeed = _buffers.v_curr[i];
			v.s_on_lane = _buffers.s_curr[i];
			v.previous_state = v.state;
			v.state = _buffers.flags[i];

			v.lateral_offset = _buffers.lateral_off[i];
			if (v.current_lane) {
				v.coordinates = lane_position(*v.current_lane, v.s_on_lane);
				v.rotationAngle = lane_rotation(*v.current_lane);
			}
		}

		for (LaneRuntime& rt : _lane_runtime) {
			rt.static_lane->vehicles.clear();
			rt.static_lane->vehicles.reserve(rt.idx.size());
			for (std::size_t idx : rt.idx) {
				rt.static_lane->vehicles.push_back(&_vehicles[idx]);
			}
		}
	}

	size_t VehicleSystem::populate() {
		if (_system.worldData().segments().empty()) {
			return 0;
		}

		auto& settings = _system.settings();
		size_t created = 0;
		auto& segment = _system.worldData().segments()[0];

		const size_t max_attempts = 100;
		size_t attempts = 0;
		while (_vehicles.size() < settings.vehiclesCount && attempts < max_attempts) {
			auto& edges = segment->road_network->edges;
			auto& edge = edges[RandomGenerator::get().next_int(0, edges.size() - 1)];
			Lane* lane = &edge.lanes[0];

			bool allowed = lane->vehicles.empty() || lane->vehicles.back()->s_on_lane > 20.0;
			if (!allowed) {
				++attempts;
				continue;
			}

			Vehicle vehicle {};
			vehicle.uid = RandomGenerator::get().next_int(1, 10000000);
			vehicle.type = RandomGenerator::get().next_enum<VehicleType>();

			auto it_config = _vehicle_configs.find(vehicle.type);
			if (it_config == _vehicle_configs.end()) {
				it_config = _vehicle_configs.begin();
			}

			vehicle.length = it_config->second.length;
			vehicle.width = it_config->second.width;
			vehicle.currentSpeed = 0;
			vehicle.maxSpeed = RandomGenerator::get().next_float(40, 100.0f);
			vehicle.coordinates = edge.start_node->coordinates;
			vehicle.currentSegmentIndex = 0;
			vehicle.current_lane = lane;
			vehicle.s_on_lane = 0.0;
			vehicle.lateral_offset = 0.0;
			VehicleStateBitsV::set_info(vehicle.state, VehicleStateBits::ST_STOPPED, VehicleStateBitsDivision::STATE);
			vehicle.previous_state = 0;
			vehicle.error = VehicleMovementError::ER_NO_ERROR;

			_vehicles.push_back(vehicle);
			insert_vehicle_sorted(*vehicle.current_lane, &_vehicles.back());

			_lane_runtime[lane->index_in_buffer].idx.push_back(_vehicles.size() - 1);

			_buffers.add_vehicle(vehicle);
			++created;
		}

		if (_vehicles.size() >= settings.vehiclesCount) {
			_creation_state = CreationState::Completed;
		}

		return created;
	}

	size_t VehicleSystem::update() {
		if (_creation_state != CreationState::InProgress) {
			return 0;
		}

		size_t created = populate();
		++_creation_ticks;
		if (_creation_ticks > 1000 && _creation_state != CreationState::Completed) {
			_creation_state = CreationState::Error;
		}

		bool need_send = created > 0 || _creation_state == CreationState::Error || _creation_state == CreationState::Completed;
		if (need_send) {
			_system.message_dispatcher().handle_message(
				core::events::VehiclesPopulated {
					created,
					_vehicles.size(),
					_system.settings().vehiclesCount,
					_creation_ticks,
					_creation_state == CreationState::Error },
				"vehicle_system");
		}

		return created;
	}

} // namespace tjs::core::simulation

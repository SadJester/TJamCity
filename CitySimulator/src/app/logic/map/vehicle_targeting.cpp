#include "stdafx.h"
#include "logic/map/vehicle_targeting.h"

#include "Application.h"

#include "data/map_renderer_data.h"

#include <visualization/elements/map_element.h>
#include <core/data_layer/world_data.h>
#include <core/store_models/vehicle_analyze_data.h>
#include <core/simulation/agent/agent_data.h>
#include <core/simulation/simulation_system.h>
#include <events/vehicle_events.h>

namespace tjs::app::logic {
	VehicleTargeting::VehicleTargeting(Application& app)
		: ILogicModule(app)
		, _maxDistance(app.settings().render.map.selectionDistance) {
	}

	void VehicleTargeting::init() {
		_application.renderer().register_event_listener(this);
	}

	void VehicleTargeting::release() {
		_application.renderer().unregister_event_listener(this);
	}

	void VehicleTargeting::on_mouse_event(const render::RendererMouseEvent& event) {
		if (event.button != render::RendererMouseEvent::ButtonType::Left || event.state != render::RendererMouseEvent::ButtonState::Pressed || !event.ctrl) {
			return;
		}

		auto* render = _application.stores().get_entry<core::model::MapRendererData>();
		auto* model = _application.stores().get_entry<core::model::VehicleAnalyzeData>();
		if (!model || !render) {
			return;
		}

		if (_application.worldData().vehicles().empty()) {
			return;
		}
		auto& vehicles = _application.worldData().vehicles();

		core::Vehicle* nearest = nullptr;
		float best_dist = _maxDistance;
		for (auto& info : vehicles) {
			FPoint node_point = visualization::convert_to_screen_f(info.coordinates, render->screen_center, render->metersPerPixel);
			float dx = static_cast<float>(node_point.x - event.x);
			float dy = static_cast<float>(node_point.y - event.y);
			float dist = dx * dx + dy * dy;
			float scaler = _application.settings().render.vehicleScaler == 0 ? 1 : _application.settings().render.vehicleScaler;
			float scaled_dist = dist / scaler;
			if (scaled_dist < best_dist) {
				best_dist = dist;
				nearest = &info;
			}
		}

		if (!nearest) {
			model->agent = nullptr;
			return;
		}

		core::simulation::TrafficSimulationSystem::Agents& agents = _application.simulationSystem().agents();
		core::AgentData* agent = nullptr;
		for (auto& a : agents) {
			if (a.vehicle == nearest) {
				agent = &a;
				break;
			}
		}

		model->agent = agent;
		_application.message_dispatcher().handle_message(
			events::AgentSelected { agent }, "map");
	}
} // namespace tjs::app::logic

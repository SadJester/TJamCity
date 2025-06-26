#include "stdafx.h"
#include "logic/map/vehicle_targeting.h"
#include "Application.h"
#include "data/persistent_render_data.h"

#include <core/store_models/vehicle_analyze_data.h>
#include <core/simulation/agent/agent_data.h>
#include <core/simulation/simulation_system.h>
#include <events/vehicle_events.h>
#include <cmath>

namespace tjs::visualization {
	VehicleTargeting::VehicleTargeting(Application& app)
		: _application(app)
		, _maxDistance(app.settings().render.map.selectionDistance) {}

	void VehicleTargeting::on_mouse_event(const render::RendererMouseEvent& event) {
		if (event.button != render::RendererMouseEvent::ButtonType::Left || event.state != render::RendererMouseEvent::ButtonState::Pressed || !event.shift) {
			return;
		}

		auto* cache = _application.stores().get_model<core::model::PersistentRenderData>();
		auto* model = _application.stores().get_model<core::model::VehicleAnalyzeData>();
		if (!cache || !model) {
			return;
		}

		core::Vehicle* nearest = nullptr;
		float best_dist = _maxDistance;
		for (auto& info : cache->vehicles) {
			float dx = static_cast<float>(info.screenPos.x - event.x);
			float dy = static_cast<float>(info.screenPos.y - event.y);
			float dist = std::sqrt(dx * dx + dy * dy);
			float scaler = _application.settings().render.vehicleScaler == 0 ? 1 : _application.settings().render.vehicleScaler;
			float scaled_dist = dist / scaler;
			if (scaled_dist < best_dist) {
				best_dist = dist;
				nearest = info.vehicle;
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
} // namespace tjs::visualization

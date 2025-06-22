#include "stdafx.h"
#include "logic/map/map_positioning.h"
#include "Application.h"
#include "data/persistent_render_data.h"
#include "data/simulation_debug_data.h"
#include "data/map_renderer_data.h"
#include <events/map_events.h>

#include <cmath>
#include <SDL3/SDL.h>
#include <algorithm>
#include <core/math_constants.h>

namespace tjs::visualization {

	MapPositioning::MapPositioning(Application& app)
		: _application(app)
		, _maxDistance(app.settings().render.map.selectionDistance) {}

	void MapPositioning::on_mouse_event(const render::RendererMouseEvent& event) {
		if (event.button != render::RendererMouseEvent::ButtonType::Left || event.state != render::RendererMouseEvent::ButtonState::Pressed) {
			return;
		}

		auto* cache = _application.stores().get_model<core::model::PersistentRenderData>();
		auto* debug = _application.stores().get_model<core::model::SimulationDebugData>();
		if (!cache || !debug) {
			return;
		}

		NodeRenderInfo* nearest = nullptr;
		float bestDist = _maxDistance;
		for (auto& [id, info] : cache->nodes) {
			float dx = static_cast<float>(info.screenPos.x - event.x);
			float dy = static_cast<float>(info.screenPos.y - event.y);
			float dist = std::sqrt(dx * dx + dy * dy);
			if (dist < bestDist) {
				bestDist = dist;
				nearest = &info;
			}
		}

		if (debug->selectedNode) {
			debug->selectedNode->selected = false;
		}

		if (nearest) {
			nearest->selected = true;
			debug->selectedNode = nearest;
		} else {
			debug->selectedNode = nullptr;
		}

		update_map_positioning();
	}

	double MapPositioning::get_changed_step(double metersPerPixel) {
		const double pixels = 50.0; // move step equals 50 screen pixels
		double meters = metersPerPixel * pixels;
		return (meters / core::MathConstants::EARTH_RADIUS) * core::MathConstants::RAD_TO_DEG;
	}

	void MapPositioning::on_mouse_wheel_event(const render::RendererMouseWheelEvent& event) {
		auto* render_data = _application.stores().get_model<core::model::MapRendererData>();
		if (!render_data) {
			return;
		}

		double oldMPP = render_data->metersPerPixel;
		double scale = event.deltaY > 0 ? 0.9 : 1.1;
		double xOff = (event.x - render_data->screen_center.x) * oldMPP;
		double yCenter = -std::log(std::tan((90.0 + render_data->projectionCenter.latitude) * core::MathConstants::DEG_TO_RAD / 2.0)) * core::MathConstants::EARTH_RADIUS;
		double yOff = (event.y - render_data->screen_center.y) * oldMPP + yCenter;
		double lonAtPoint = render_data->projectionCenter.longitude + xOff / (core::MathConstants::EARTH_RADIUS * core::MathConstants::DEG_TO_RAD);
		double latAtPoint = (2.0 * std::atan(std::exp(-yOff / core::MathConstants::EARTH_RADIUS)) - core::MathConstants::M_PI / 2.0) * core::MathConstants::RAD_TO_DEG;
		double newMPP = oldMPP * scale;
		render_data->set_meters_per_pixel(newMPP);
		double yPoint = -std::log(std::tan((90.0 + latAtPoint) * core::MathConstants::DEG_TO_RAD / 2.0)) * core::MathConstants::EARTH_RADIUS;
		double newYCenter = yPoint - (event.y - render_data->screen_center.y) * newMPP;
		double newLat = (2.0 * std::atan(std::exp(-newYCenter / core::MathConstants::EARTH_RADIUS)) - core::MathConstants::M_PI / 2.0) * core::MathConstants::RAD_TO_DEG;
		double newLon = lonAtPoint - (event.x - render_data->screen_center.x) * newMPP / (core::MathConstants::EARTH_RADIUS * core::MathConstants::DEG_TO_RAD);
		render_data->projectionCenter.latitude = std::clamp(newLat, -90.0, 90.0);
		render_data->projectionCenter.longitude = std::clamp(newLon, -180.0, 180.0);
		render_data->projectionCenter.x = render_data->projectionCenter.longitude * core::MathConstants::DEG_TO_RAD * core::MathConstants::EARTH_RADIUS;
		render_data->projectionCenter.y = -std::log(std::tan((90.0 + render_data->projectionCenter.latitude) * core::MathConstants::DEG_TO_RAD / 2.0)) * core::MathConstants::EARTH_RADIUS;

		update_map_positioning();
	}

	void MapPositioning::on_key_event(const render::RendererKeyEvent& event) {
		if (event.state != render::RendererKeyEvent::KeyState::Pressed) {
			return;
		}

		auto* render_data = _application.stores().get_model<core::model::MapRendererData>();
		if (!render_data) {
			return;
		}

		core::Coordinates current = render_data->projectionCenter;
		double step = get_changed_step(render_data->metersPerPixel);
		double lonStep = step / std::cos(current.latitude * core::MathConstants::DEG_TO_RAD);

		switch (event.keyCode) {
			case SDLK_UP:
				if (current.latitude < 90.0) {
					current.latitude += step;
				}
				break;
			case SDLK_DOWN:
				if (current.latitude > -90.0) {
					current.latitude -= step;
				}
				break;
			case SDLK_LEFT:
				if (current.longitude > -180.0) {
					current.longitude -= lonStep;
				}
				break;
			case SDLK_RIGHT:
				if (current.longitude < 180.0) {
					current.longitude += lonStep;
				}
				break;
			default:
				return;
		}

		render_data->projectionCenter = current;
		render_data->projectionCenter.x = current.longitude * core::MathConstants::DEG_TO_RAD * core::MathConstants::EARTH_RADIUS;
		render_data->projectionCenter.y = -std::log(std::tan((90.0 + current.latitude) * core::MathConstants::DEG_TO_RAD / 2.0)) * core::MathConstants::EARTH_RADIUS;

		update_map_positioning();
	}

	void MapPositioning::update_map_positioning() {
		auto* render_data = _application.stores().get_model<core::model::MapRendererData>();
		if (render_data) {
			visualization::recalculate_map_data(_application);
		}

		auto& general_settings = _application.settings().general;
		general_settings.projectionCenter = render_data->projectionCenter;
		general_settings.zoomLevel = render_data->metersPerPixel;
		_application.message_dispatcher().handle_message(events::MapPositioningChanged {}, "map");
	}

} // namespace tjs::visualization

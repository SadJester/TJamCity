#include "stdafx.h"
#include "logic/map/map_positioning.h"
#include "Application.h"
#include "data/persistent_render_data.h"
#include "data/map_renderer_data.h"
#include <events/map_events.h>

#include <visualization/elements/map_element.h>

#include <core/math_constants.h>
#include <core/data_layer/world_data.h>
#include <core/simulation/simulation_debug.h>

#include <SDL3/SDL.h>

namespace tjs::app::logic {

	MapPositioning::MapPositioning(Application& app)
		: ILogicModule(app)
		, _maxDistance(app.settings().render.map.selectionDistance) {
	}

	void MapPositioning::init() {
		_application.renderer().register_event_listener(this);
	}

	void MapPositioning::release() {
		_application.renderer().unregister_event_listener(this);
	}

	void MapPositioning::on_mouse_event(const render::RendererMouseEvent& event) {
		if (event.button != render::RendererMouseEvent::ButtonType::Left) {
			return;
		}

		if (event.state == render::RendererMouseEvent::ButtonState::Released) {
			_dragging = false;
			return;
		}

		_dragging = true;

		auto* debug = &_application.settings().simulationSettings.debug_data;
		auto* render = _application.stores().get_entry<core::model::MapRendererData>();
		if (!debug || !render) {
			return;
		}

		if (_application.worldData().segments().empty()) {
			return;
		}
		auto& ways = _application.worldData().segments().front()->ways;

		core::Node* nearest = nullptr;
		const double squared_max_dist = _maxDistance * _maxDistance;
		float best_dist = squared_max_dist;

		for (auto& way_pair : ways) {
			for (auto node : way_pair.second->nodes) {
				FPoint node_point = visualization::convert_to_screen_f(node->coordinates, render->screen_center, render->metersPerPixel);
				float dx = static_cast<float>(node_point.x - event.x);
				float dy = static_cast<float>(node_point.y - event.y);
				float squared_dist = dx * dx + dy * dy;
				if (squared_dist < best_dist) {
					best_dist = squared_dist;
					nearest = node;
				}
			}
		}

		debug->selectedNode = nearest;
		update_map_positioning();
	}

	void MapPositioning::on_mouse_wheel_event(const render::RendererMouseWheelEvent& event) {
		auto* render_data = _application.stores().get_entry<core::model::MapRendererData>();
		if (!render_data) {
			return;
		}

		double oldMPP = render_data->metersPerPixel;
		double scale = event.deltaY > 0 ? 0.9 : 1.1;
		double worldX = (event.x - render_data->screen_center.x) * oldMPP;
		double worldY = (event.y - render_data->screen_center.y) * oldMPP;
		double newMPP = oldMPP * scale;
		render_data->set_meters_per_pixel(newMPP);
		render_data->screen_center.x = static_cast<int>(event.x - worldX / newMPP);
		render_data->screen_center.y = static_cast<int>(event.y - worldY / newMPP);

		update_map_positioning();
	}

	void MapPositioning::on_mouse_motion_event(const render::RendererMouseMotionEvent& event) {
		if (!_dragging) {
			return;
		}

		auto* render_data = _application.stores().get_entry<core::model::MapRendererData>();
		if (!render_data) {
			return;
		}

		render_data->screen_center.x += event.xrel;
		render_data->screen_center.y += event.yrel;

		update_map_positioning();
	}

	void MapPositioning::on_key_event(const render::RendererKeyEvent& event) {
		if (event.state != render::RendererKeyEvent::KeyState::Pressed) {
			return;
		}

		auto* render_data = _application.stores().get_entry<core::model::MapRendererData>();
		if (!render_data) {
			return;
		}

		int step = 50; // pixels to move

		switch (event.keyCode) {
			case SDLK_UP:
				render_data->screen_center.y += step;
				break;
			case SDLK_DOWN:
				render_data->screen_center.y -= step;
				break;
			case SDLK_LEFT:
				render_data->screen_center.x += step;
				break;
			case SDLK_RIGHT:
				render_data->screen_center.x -= step;
				break;
			default:
				return;
		}

		update_map_positioning();
	}

	void MapPositioning::update_map_positioning() {
		auto* render_data = _application.stores().get_entry<core::model::MapRendererData>();
		if (render_data) {
			visualization::recalculate_map_data(_application);
		}

		auto& general_settings = _application.settings().general;
		general_settings.screen_center = render_data->screen_center;
		general_settings.zoomLevel = render_data->metersPerPixel;
		_application.message_dispatcher().handle_message(events::MapPositioningChanged {}, "map");
	}

} // namespace tjs::app::logic

#include <stdafx.h>

#include <visual_system/logic/map/map_positioning.h>
#include <visual_system/data/map_renderer_data.h>
#include <visual_system/visual_system.h>

#include <data/persistent_render_data.h>

#include <visualization/elements/map_element.h>

#include <Application.h>

#include <core/math_constants.h>
#include <core/data_layer/world_data.h>
#include <core/simulation/simulation_debug.h>

#include <SDL3/SDL.h>

namespace tjs::app::logic {

	MapPositioning::MapPositioning(Application& app)
		: ILogicModule(app)
		, _maxDistance(app.settings().render.map.selectionDistance)
		, _render_data(*_application.stores().get_entry<core::model::MapRendererShared>()) {
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
		auto* render = _application.stores().get_entry<core::model::MapRendererShared>()->get();
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
				FPoint node_point = visualization::convert_to_screen_f(node->coordinates, render->get_screen_center(), render->get_meters_ppx());
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
		double oldMPP = _render_data->get_meters_ppx();
		double scale = event.deltaY > 0 ? 0.9 : 1.1;
		double worldX = (event.x - _render_data->get_screen_center().x) * oldMPP;
		double worldY = (event.y - _render_data->get_screen_center().y) * oldMPP;
		double newMPP = oldMPP * scale;
		_render_data->set_meters_ppx(newMPP);
		_render_data->set_screen_center({ static_cast<int>(event.x - worldX / newMPP),
			static_cast<int>(event.y - worldY / newMPP) });

		update_map_positioning();
	}

	void MapPositioning::on_mouse_motion_event(const render::RendererMouseMotionEvent& event) {
		if (!_dragging) {
			return;
		}

		Position pos = _render_data->get_screen_center();
		pos.x += event.xrel;
		pos.y += event.yrel;
		_render_data->set_screen_center(pos);

		update_map_positioning();
	}

	void MapPositioning::on_key_event(const render::RendererKeyEvent& event) {
		if (event.state != render::RendererKeyEvent::KeyState::Pressed) {
			return;
		}

		Position pos = _render_data->get_screen_center();

		int step = 50; // pixels to move

		switch (event.keyCode) {
			case SDLK_UP:
				pos.y += step;
				break;
			case SDLK_DOWN:
				pos.y -= step;
				break;
			case SDLK_LEFT:
				pos.x += step;
				break;
			case SDLK_RIGHT:
				pos.x -= step;
				break;
			default:
				return;
		}

		_render_data->set_screen_center(pos);
		update_map_positioning();
	}

	void MapPositioning::update_map_positioning() {
		visualization::recalculate_map_data(_application);

		// TODO{threaded}: Listener in main thread that saves settings
		auto& general_settings = _application.settings().general;
		general_settings.screen_center = _render_data->get_screen_center();
		general_settings.zoomLevel = _render_data->get_meters_ppx();

		auto& v_sys = *_application.systems().get<visualization::VisualSystem>();
		v_sys.optional_qeueue().push<visualization::VisualSystemEvents::map_position_changed>();
	}

} // namespace tjs::app::logic

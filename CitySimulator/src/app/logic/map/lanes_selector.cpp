#include <stdafx.h>
#include <logic/map/lanes_selector.h>
#include <Application.h>
#include <data/map_renderer_data.h>
#include <visualization/elements/map_element.h>

#include <core/data_layer/world_data.h>
#include <core/simulation/simulation_debug.h>
#include <events/map_events.h>

namespace tjs::app::logic {

	static float point_segment_distance(const Position& p, const Position& a, const Position& b) {
		float dx = static_cast<float>(b.x - a.x);
		float dy = static_cast<float>(b.y - a.y);
		float l2 = dx * dx + dy * dy;
		if (l2 == 0.0f) {
			float dxp = static_cast<float>(p.x - a.x);
			float dyp = static_cast<float>(p.y - a.y);
			return std::sqrt(dxp * dxp + dyp * dyp);
		}
		float t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / l2;
		t = std::clamp(t, 0.0f, 1.0f);
		float projx = a.x + t * dx;
		float projy = a.y + t * dy;
		float dxp = static_cast<float>(p.x - projx);
		float dyp = static_cast<float>(p.y - projy);
		return std::sqrt(dxp * dxp + dyp * dyp);
	}

	LanesSelector::LanesSelector(Application& app)
		: ILogicModule(app)
		, _maxDistance(app.settings().render.map.selectionDistance) {
	}

	void LanesSelector::init() {
		_application.renderer().register_event_listener(this);
	}

	void LanesSelector::release() {
		_application.renderer().unregister_event_listener(this);
	}

	void LanesSelector::on_mouse_event(const render::RendererMouseEvent& event) {
		if (event.button != render::RendererMouseEvent::ButtonType::Left || event.state != render::RendererMouseEvent::ButtonState::Pressed) {
			return;
		}

		auto* render_data = _application.stores().get_entry<core::model::MapRendererData>();
		auto* debug_data = &_application.settings().simulationSettings.debug_data;
		if (!render_data || !debug_data) {
			return;
		}

		if (_application.worldData().segments().empty()) {
			render_data->selected_lane = nullptr;
			return;
		}

		core::WorldSegment& segment = *_application.worldData().segments().front();
		if (!segment.road_network) {
			render_data->selected_lane = nullptr;
			return;
		}

		Position click { event.x, event.y };
		core::Lane* nearestLane = nullptr;
		float bestDist = _maxDistance / render_data->metersPerPixel;

		for (const core::Edge& edge : segment.road_network->edges) {
			for (const core::Lane& lane : edge.lanes) {
				if (lane.centerLine.size() < 2) {
					continue;
				}
				for (size_t i = 0; i + 1 < lane.centerLine.size(); ++i) {
					Position a = visualization::convert_to_screen(
						lane.centerLine[i], render_data->screen_center, render_data->metersPerPixel);
					Position b = visualization::convert_to_screen(
						lane.centerLine[i + 1], render_data->screen_center, render_data->metersPerPixel);
					float dist = point_segment_distance(click, a, b);
					if (dist < bestDist) {
						bestDist = dist;
						nearestLane = const_cast<core::Lane*>(&lane);
					}
				}
			}
		}

		core::Node* nearest_node = nullptr;
		for (auto& n : segment.road_network->nodes) {
			Position p = visualization::convert_to_screen(n.second->coordinates, render_data->screen_center, render_data->metersPerPixel);

			float dx = static_cast<float>(p.x - event.x);
			float dy = static_cast<float>(p.y - event.y);
			float dist = std::sqrt(dx * dx + dy * dy);
			if (dist < bestDist) {
				bestDist = dist;
				nearestLane = nullptr;
				nearest_node = n.second;
			}
		}

		
		debug_data->selectedNode = nearest_node;
		render_data->selected_lane = nearestLane;
		_application.message_dispatcher().handle_message(events::LaneIsSelected {}, "map");
	}

} // namespace tjs::app::logic

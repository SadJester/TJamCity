#include "stdafx.h"
#include "visualization/map_render_events_listener.h"
#include "Application.h"
#include "data/persistent_render_data.h"

#include <cmath>

namespace tjs::visualization {

	MapRenderEventsListener::MapRenderEventsListener(Application& app)
		: _application(app)
		, _maxDistance(app.settings().render.map.selectionDistance) {}

	void MapRenderEventsListener::on_mouse_event(const render::RendererMouseEvent& event) {
		if (event.button != render::RendererMouseEvent::ButtonType::Left || event.state != render::RendererMouseEvent::ButtonState::Pressed) {
			return;
		}

		auto* cache = _application.stores().get_model<core::model::PersistentRenderData>();
		if (!cache) {
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

		if (cache->selectedNode) {
			cache->selectedNode->selected = false;
		}

		if (nearest) {
			nearest->selected = true;
			cache->selectedNode = nearest;
		} else {
			cache->selectedNode = nullptr;
		}
	}

} // namespace tjs::visualization

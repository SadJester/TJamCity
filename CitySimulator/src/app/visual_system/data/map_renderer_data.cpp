#include <stdafx.h>

#include <visual_system/data/map_renderer_data.h>

namespace tjs::core::model {

	void MapRendererData::set_meters_ppx(double meters_ppx) {
		this->_meters_per_pixel = meters_ppx;
		_changed = true;
	}

	void MapRendererData::set_show_bounding_box(bool value) {
		_show_bounding_box = value;
		_changed = true;
	}

	void MapRendererData::set_show_lane_markers(bool value) {
		_show_lane_markers = value;
		_changed = true;
	}

	void MapRendererData::set_lane_marker_visibility_threshold(double value) {
		_lane_marker_visibility_threshold = value;
		_changed = true;
	}

	void MapRendererData::set_simplified_view_threshold(double value) {
		_simplified_view_threshold = value;
		_changed = true;
	}

	void MapRendererData::set_network_only_for_selected(bool value) {
		_network_only_for_selected = value;
		_changed = true;
	}

	void MapRendererData::set_selected_lane(core::Lane* value) {
		_selected_lane = value;
		_changed = true;
	}

	void MapRendererData::set_screen_center(Position value) {
		_screen_center = value;
		_changed = true;
	}

	void MapRendererData::set_visible_layers(MapRendererLayer value) {
		_visible_layers = value;
		_changed = true;
	}

	void MapRendererData::sync(MapRendererData& dst, const MapRendererData& src) {
		dst._meters_per_pixel = src._meters_per_pixel;
		dst._show_bounding_box = src._show_bounding_box;
		dst._lane_marker_visibility_threshold = src._lane_marker_visibility_threshold;
		dst._simplified_view_threshold = src._simplified_view_threshold;

		dst._network_only_for_selected = src._network_only_for_selected;
		dst._selected_lane = src._selected_lane;
		dst._screen_center = src._screen_center;
		dst._visible_layers = src._visible_layers;
	}

} // namespace tjs::core::model

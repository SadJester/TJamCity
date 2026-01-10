#pragma once

#include <core/store_models/idata_model.h>
#include <render/render_primitives.h>

#include <common/sync/shared_state.h>

namespace tjs::core {
	struct Lane;
} // namespace tjs::core

namespace tjs::core::model {

	ENUM_FLAG(MapRendererLayer, char,
		None = 0,
		Ways = 1 << 0,
		Nodes = 1 << 1,
		TrafficLights = 1 << 2,
		NetworkGraph = 1 << 3,
		All = Ways | Nodes | TrafficLights | NetworkGraph);

	struct MapRendererData : public IDataModel {
		static std::type_index get_type() {
			return typeid(MapRendererData);
		}

		MapRendererData() = default;

		void reinit() override {
			_selected_lane = nullptr;
			_changed = true;
		}

		bool is_changed() const { return _changed; }
		void reset_changed() { _changed = false; }

		void set_meters_ppx(double meters_ppx);
		double get_meters_ppx() const { return _meters_per_pixel; }

		void set_show_bounding_box(bool value);
		bool get_show_bounding_box() const { return _show_bounding_box; }

		void set_show_lane_markers(bool value);
		bool get_show_lane_markers() const { return _show_lane_markers; }

		void set_lane_marker_visibility_threshold(double value);
		double get_lane_marker_visibility_threshold() const { return _lane_marker_visibility_threshold; }

		void set_simplified_view_threshold(double value);
		double get_simplified_view_threshold() const { return _simplified_view_threshold; }

		void set_network_only_for_selected(bool value);
		bool get_network_only_for_selected() const { return _network_only_for_selected; }

		void set_selected_lane(core::Lane* value);
		core::Lane* get_selected_lane() const { return _selected_lane; }

		void set_screen_center(Position value);
		Position get_screen_center() const { return _screen_center; }

		void set_visible_layers(MapRendererLayer value);
		MapRendererLayer get_visible_layers() const { return _visible_layers; }

		static void sync(MapRendererData& dst, const MapRendererData& src);

	private:
		Position _screen_center;

		// View settings
		double _meters_per_pixel = 1.0;
		double _lane_marker_visibility_threshold = 1.0; // meters per pixel threshold
		double _simplified_view_threshold = 1.2;        // meters per pixel threshold

		core::Lane* _selected_lane = nullptr;

		// Layer visibility flags
		MapRendererLayer _visible_layers = MapRendererLayer::Ways;

		// Rendering options
		bool _show_bounding_box = false;
		bool _show_lane_markers = true;
		bool _network_only_for_selected = false;
		bool _changed = false;
	};

	struct MapRendererShared : public IDataModel, common::sync::shared_data<MapRendererData, 3> {
		static std::type_index get_type() {
			return typeid(MapRendererShared);
		}

		MapRendererShared() = default;

		void reinit() override {
			(*this)->reinit();
		}
	};

} // namespace tjs::core::model

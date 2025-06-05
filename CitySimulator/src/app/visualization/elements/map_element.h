#pragma once

#include <visualization/scene_node.h>
#include <core/data_layer/data_types.h>
#include <data/map_renderer_data.h>

namespace tjs {
	class Application;
	class IRenderer;
} // namespace tjs

namespace tjs::visualization {
	class MapElement : public SceneNode {
	public:
		explicit MapElement(Application& application);

		void init() override;
		void update() override;
		void render(IRenderer& renderer) override;

	private:
		Position convert_to_screen(const core::Coordinates& coord) const;
		void auto_zoom(const std::unordered_map<uint64_t, std::unique_ptr<core::Node>>& nodes);
		void calculate_map_bounds(const std::unordered_map<uint64_t, std::unique_ptr<core::Node>>& nodes);
		int render_way(const core::WayInfo& way, const std::unordered_map<uint64_t, std::unique_ptr<core::Node>>& nodes);
		void render_bounding_box() const;
		void draw_lane_markers(const std::vector<Position>& nodes, int lanes, int lane_width_pixels);
		void draw_path_nodes(const std::vector<Position>& nodes);
		FColor get_way_color(core::WayType type) const;

		Application& _application;
		core::model::MapRendererData& _render_data;

		// Bounding box coordinates
		float min_lat = 0.0f;
		float max_lat = 0.0f;
		float min_lon = 0.0f;
		float max_lon = 0.0f;

		// Screen center coordinates
		int screen_center_x = 512;
		int screen_center_y = 512;
	};

	int drawThickLine(IRenderer& renderer, const std::vector<Position>& nodes, double metersPerPixel, float thickness, FColor color);
	Position convert_to_screen(
		const core::Coordinates& coord,
		const core::Coordinates& projection_center,
		const Position& screen_center,
		double meters_per_pixel);
} // namespace tjs::visualization

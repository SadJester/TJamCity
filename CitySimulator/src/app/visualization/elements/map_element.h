#pragma once

#include <visualization/scene_node.h>
#include <core/data_layer/data_types.h>
#include <data/map_renderer_data.h>
#include <data/persistent_render_data.h>
#include <data/simulation_debug_data.h>
#include <core/data_layer/road_network.h>

#include <events/project_events.h>

namespace tjs {
	class Application;
	class IRenderer;
} // namespace tjs

namespace tjs::visualization {
	class MapElement final : public SceneNode {
	public:
		explicit MapElement(Application& application);
		~MapElement();

		void init() override;
		void update() override;
		void render(IRenderer& renderer) override;

		void on_map_updated();

	private:
		void handle_open_map_simulation_reinit(const events::OpenMapEvent& event);

	private:
		Position convert_to_screen(const core::Coordinates& coord) const;
		void auto_zoom(const std::unordered_map<uint64_t, std::unique_ptr<core::Node>>& nodes);
		void calculate_map_bounds(const std::unordered_map<uint64_t, std::unique_ptr<core::Node>>& nodes);
		void render_bounding_box() const;
		void draw_lane_markers(const std::vector<Position>& nodes, int lanes, int lane_width_pixels);
		void render_network_graph(IRenderer& renderer, const core::RoadNetwork& network);

		Application& _application;
		core::model::MapRendererData& _render_data;
		core::model::PersistentRenderData& _cache;
		core::model::SimulationDebugData& _debugData;

		// Bounding box coordinates
		float min_lat = 0.0f;
		float max_lat = 0.0f;
		float min_lon = 0.0f;
		float max_lon = 0.0f;
		double min_x = 0.0;
		double max_x = 0.0;
		double min_y = 0.0;
		double max_y = 0.0;

		std::string _current_file;
	};

	int drawThickLine(IRenderer& renderer, const std::vector<Position>& nodes, double metersPerPixel, float thickness, FColor color);
	Position convert_to_screen(
		const core::Coordinates& coord,
		const Position& screen_center,
		double meters_per_pixel);
} // namespace tjs::visualization

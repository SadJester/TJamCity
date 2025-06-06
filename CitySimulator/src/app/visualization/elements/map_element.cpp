#include "stdafx.h"

#include "visualization/elements/map_element.h"

#include <render/render_base.h>
#include <visualization/visualization_constants.h>
#include <data/map_renderer_data.h>
#include <Application.h>

#include <core/data_layer/world_data.h>
#include <core/data_layer/data_types.h>
#include <core/math_constants.h>

namespace tjs::visualization {
	using namespace tjs::core;

	MapElement::MapElement(Application& application)
		: SceneNode("MapElement")
		, _application(application)
		, _render_data(*application.stores().get_model<model::MapRendererData>()) {
	}

	void MapElement::init() {
		auto& world = _application.worldData();
		auto& segments = world.segments();

		if (!segments.empty()) {
			auto_zoom(segments.front()->nodes);
		}
	}

	void MapElement::update() {
	}

	void MapElement::render(IRenderer& renderer) {
		auto& world = _application.worldData();
		auto& segments = world.segments();

		if (segments.empty()) {
			return;
		}

		auto& segment = segments.front();

		if (_render_data.showBoundingBox) {
			render_bounding_box();
		}

		// Render all ways if enabled
		if (static_cast<uint32_t>(_render_data.visibleLayers & model::MapRendererLayer::Ways) != 0) {
			int ways_rendered = 0;
			int total_nodes_rendered = 0;
			int real_node_segments = 0;
			for (const auto& way_pair : segment->ways) {
				std::vector<Position> screen_points;
				screen_points.reserve(way_pair.second->nodes.size());

				for (Node* node : way_pair.second->nodes) {
					screen_points.push_back(convert_to_screen(node->coordinates));
				}

				int nodes_rendered = render_way(*way_pair.second, segment->nodes);
				real_node_segments += way_pair.second->nodes.size() / 2;
				if (nodes_rendered > 0) {
					ways_rendered++;
				}
				total_nodes_rendered += nodes_rendered;

				// Draw nodes if enabled
				if (static_cast<uint32_t>(_render_data.visibleLayers & model::MapRendererLayer::Nodes) != 0) {
					draw_path_nodes(screen_points);
				}
			}
		}

		// Render network graph if enabled
		if (static_cast<uint32_t>(_render_data.visibleLayers & model::MapRendererLayer::NetworkGraph) != 0) {
			if (segment->road_network) {
				render_network_graph(renderer, *segment->road_network);
			}
		}
	}

	void MapElement::render_network_graph(IRenderer& renderer, const core::RoadNetwork& network) {
		// Set color for network graph edges
		renderer.set_draw_color({ 0.0f, 0.8f, 0.8f, 0.5f }); // Semi-transparent cyan

		// Render edges from adjacency list
		for (const auto& [node, neighbors] : network.adjacency_list) {
			Position start = convert_to_screen(node->coordinates);

			for (const auto& [neighbor, weight] : neighbors) {
				Position end = convert_to_screen(neighbor->coordinates);

				// Draw edge as a thin line
				drawThickLine(renderer, { start, end }, _render_data.metersPerPixel, 1.0f, { 0.0f, 0.8f, 0.8f, 0.5f });
			}
		}

		// Draw nodes as small circles
		renderer.set_draw_color({ 1.0f, 0.0f, 0.0f, 0.8f }); // Brighter cyan for nodes
		for (const auto& [id, node] : network.nodes) {
			Position pos = convert_to_screen(node->coordinates);
			renderer.draw_circle(pos.x, pos.y, 2, true);
		}
	}

	Position convert_to_screen(
		const Coordinates& coord,
		const Coordinates& projection_center,
		const Position& screen_center,
		double meters_per_pixel) {
		// Convert geographic coordinates to meters using Mercator projection
		double x = (coord.longitude - projection_center.longitude) * MathConstants::DEG_TO_RAD * MathConstants::EARTH_RADIUS;
		double y = -std::log(std::tan((90.0 + coord.latitude) * MathConstants::DEG_TO_RAD / 2.0)) * MathConstants::EARTH_RADIUS;
		double yCenter = -std::log(std::tan((90.0 + projection_center.latitude) * MathConstants::DEG_TO_RAD / 2.0)) * MathConstants::EARTH_RADIUS;
		y -= yCenter;

		// Scale to screen coordinates
		int screenX = static_cast<int>(screen_center.x + x / meters_per_pixel);
		int screenY = static_cast<int>(screen_center.y + y / meters_per_pixel);

		return { screenX, screenY };
	}

	Position MapElement::convert_to_screen(const Coordinates& coord) const {
		return tjs::visualization::convert_to_screen(
			coord,
			_render_data.projectionCenter,
			_render_data.screen_center,
			_render_data.metersPerPixel);
	}

	void MapElement::auto_zoom(const std::unordered_map<uint64_t, std::unique_ptr<Node>>& nodes) {
		if (nodes.empty()) {
			return;
		}

		calculate_map_bounds(nodes);

		// Calculate bounds in meters
		double minX = std::numeric_limits<double>::max();
		double maxX = std::numeric_limits<double>::lowest();
		double minY = std::numeric_limits<double>::max();
		double maxY = std::numeric_limits<double>::lowest();

		double yCenter = -std::log(std::tan((90.0 + _render_data.projectionCenter.latitude) * MathConstants::DEG_TO_RAD / 2.0)) * MathConstants::EARTH_RADIUS;

		for (const auto& pair : nodes) {
			const auto& node = pair.second;
			double x = (node->coordinates.longitude - _render_data.projectionCenter.longitude) * MathConstants::DEG_TO_RAD * MathConstants::EARTH_RADIUS;
			double y = -std::log(std::tan((90.0 + node->coordinates.latitude) * MathConstants::DEG_TO_RAD / 2.0)) * MathConstants::EARTH_RADIUS - yCenter;

			minX = std::min(minX, x);
			maxX = std::max(maxX, x);
			minY = std::min(minY, y);
			maxY = std::max(maxY, y);
		}

		// Calculate required zoom level
		double widthMeters = maxX - minX;
		double heightMeters = maxY - minY;

		auto& renderer = _application.renderer();

		double zoomX = widthMeters / (renderer.screen_width() * 0.9);
		double zoomY = heightMeters / (renderer.screen_height() * 0.9);

		_render_data.set_meters_per_pixel(std::min(zoomX, zoomY));

		// Recalculate screen center based on the new zoom level
		_render_data.screen_center.x = renderer.screen_width() / 2.0;
		_render_data.screen_center.y = renderer.screen_height() / 2.0;
	}

	void MapElement::calculate_map_bounds(const std::unordered_map<uint64_t, std::unique_ptr<Node>>& nodes) {
		// Initialize bounding box with extreme values
		min_lat = std::numeric_limits<float>::max();
		max_lat = std::numeric_limits<float>::lowest();
		min_lon = std::numeric_limits<float>::max();
		max_lon = std::numeric_limits<float>::lowest();

		// Iterate through all nodes to find min/max coordinates
		for (const auto& pair : nodes) {
			const auto& node = pair.second;

			min_lat = std::min(min_lat, static_cast<float>(node->coordinates.latitude));
			max_lat = std::max(max_lat, static_cast<float>(node->coordinates.latitude));
			min_lon = std::min(min_lon, static_cast<float>(node->coordinates.longitude));
			max_lon = std::max(max_lon, static_cast<float>(node->coordinates.longitude));
		}

		// Calculate the center of the bounding box
		_render_data.projectionCenter.latitude = (min_lat + max_lat) / 2.0f;
		_render_data.projectionCenter.longitude = (min_lon + max_lon) / 2.0f;
	}

	FColor MapElement::get_way_color(WayType type) const {
		FColor roadColor = Constants::ROAD_COLOR;
		switch (type) {
			case WayType::Motorway:
				roadColor = Constants::MOTORWAY_COLOR;
				break;
			case WayType::Primary:
				roadColor = Constants::PRIMARY_COLOR;
				break;
			case WayType::Residential:
				roadColor = Constants::RESIDENTIAL_COLOR;
				break;
			case WayType::Steps:
				roadColor = Constants::STEPS_COLOR;
				break;
			case WayType::Construction:
				roadColor = Constants::CONSTRUCTION_COLOR;
				break;
			case WayType::Raceway:
				roadColor = Constants::RACEWAY_COLOR;
				break;
			case WayType::Emergency_Bay:
			case WayType::Emergency_Access:
			case WayType::Parking:
				roadColor = Constants::EMERGENCY_COLOR;
				break;
			case WayType::Rest_Area:
			case WayType::Services:
				roadColor = Constants::SERVICE_AREA_COLOR;
				break;
			case WayType::Bus_Stop:
			case WayType::Bus_Guideway:
				roadColor = Constants::BUS_STOP_COLOR;
				break;
			default:
				break;
		}
		return roadColor;
	}

	int MapElement::render_way(const WayInfo& way, const std::unordered_map<uint64_t, std::unique_ptr<Node>>& nodes) {
		if (way.nodeRefs.size() < 2) {
			return 0;
		}

		std::vector<Position> screenPoints;
		screenPoints.reserve(way.nodes.size());

		for (Node* node : way.nodes) {
			screenPoints.push_back(convert_to_screen(node->coordinates));
		}

		// Draw the way
		if (screenPoints.size() < 2) {
			return 0;
		}

		bool hasVisiblePoints = false;
		for (const auto& point : screenPoints) {
			if (_application.renderer().is_point_visible(point.x, point.y)) {
				hasVisiblePoints = true;
				break;
			}
		}
		if (!hasVisiblePoints) {
			return 0;
		}

		const FColor color = get_way_color(way.type);
		const float lane_width = way.is_car_accessible() ? way.lanes * Constants::LANE_WIDTH : Constants::LANE_WIDTH / 2;

		int segmentsRendered = drawThickLine(_application.renderer(), screenPoints, _render_data.metersPerPixel, lane_width, color);

		if (way.lanes > 1) {
			draw_lane_markers(screenPoints, way.lanes, Constants::LANE_WIDTH);
		}

		// Draw direction arrows if it's a one-way road or has explicit lane directions
		if (way.is_car_accessible() && (way.isOneway || (way.lanesForward > 0 && way.lanesBackward == 0) || (way.lanesForward == 0 && way.lanesBackward > 0))) {
			draw_direction_arrows(screenPoints, way.isOneway && way.lanesBackward > 0);
		}

		return segmentsRendered;
	}

	void MapElement::draw_direction_arrows(const std::vector<Position>& nodes, bool reverse) {
		if (_render_data.metersPerPixel > Constants::DRAW_LANE_MARKERS_MPP || nodes.size() < 2) {
			return;
		}

		auto& renderer = _application.renderer();
		renderer.set_draw_color(Constants::LANE_MARKER_COLOR);

		for (size_t i = 0; i < nodes.size() - 1; i++) {
			Position p1 = nodes[i];
			Position p2 = nodes[i + 1];

			// Calculate direction vector
			float dx = p2.x - p1.x;
			float dy = p2.y - p1.y;
			float len = sqrtf(dx * dx + dy * dy);
			if (len == 0) {
				continue;
			}

			// Normalize direction vector
			dx /= len;
			dy /= len;

			// Draw arrows along the segment
			float arrow_spacing = 30.0f; // pixels
			int num_arrows = static_cast<int>(len / arrow_spacing);

			for (int j = 1; j < num_arrows; j++) {
				// Calculate arrow center position
				float t = j * arrow_spacing;
				Position arrow_center;
				if (!reverse) {
					arrow_center = {
						static_cast<int>(p1.x + t * dx),
						static_cast<int>(p1.y + t * dy)
					};
				} else {
					arrow_center = {
						static_cast<int>(p2.x - t * dx),
						static_cast<int>(p2.y - t * dy)
					};
				}

				// Skip if arrow center is not visible
				if (!_application.renderer().is_point_visible(arrow_center.x, arrow_center.y)) {
					continue;
				}

				// Calculate arrow points
				float arrow_size = 5.0f; // pixels
				float arrow_angle = std::atan2(dy, dx);
				if (reverse) {
					arrow_angle += MathConstants::M_PI; // Reverse direction for backward lanes
				}
				float arrow_angle1 = arrow_angle - 0.5f; // 30 degrees
				float arrow_angle2 = arrow_angle + 0.5f; // 30 degrees

				Position arrow_p1 = {
					static_cast<int>(arrow_center.x - arrow_size * std::cos(arrow_angle1)),
					static_cast<int>(arrow_center.y - arrow_size * std::sin(arrow_angle1))
				};
				Position arrow_p2 = {
					static_cast<int>(arrow_center.x - arrow_size * std::cos(arrow_angle2)),
					static_cast<int>(arrow_center.y - arrow_size * std::sin(arrow_angle2))
				};

				// Draw arrow
				renderer.draw_line(arrow_center.x, arrow_center.y, arrow_p1.x, arrow_p1.y);
				renderer.draw_line(arrow_center.x, arrow_center.y, arrow_p2.x, arrow_p2.y);
			}
		}
	}

	int drawThickLine(IRenderer& renderer, const std::vector<Position>& nodes, double metersPerPixel, float thickness, FColor color) {
		if (nodes.size() < 2) {
			return 0;
		}

		thickness /= metersPerPixel;

		int segmentsRendered = 0;
		renderer.set_draw_color(color);

		for (size_t i = 0; i < nodes.size() - 1; i++) {
			auto p1 = nodes[i];
			auto p2 = nodes[i + 1];

			// Calculate perpendicular vector
			float dx = p2.x - p1.x;
			float dy = p2.y - p1.y;
			float len = sqrtf(dx * dx + dy * dy);
			if (len == 0) {
				continue;
			}
			++segmentsRendered;
			float perpx = -dy / len * thickness / 2;
			float perpy = dx / len * thickness / 2;

			// Draw a thick line as a quad
			Vertex vertices[4] = {
				{ { p1.x + perpx, p1.y + perpy }, color, { 0.f, 0.f } }, // top-left
				{ { p1.x - perpx, p1.y - perpy }, color, { 0.f, 0.f } }, // bottom-left
				{ { p2.x - perpx, p2.y - perpy }, color, { 0.f, 0.f } }, // bottom-right
				{ { p2.x + perpx, p2.y + perpy }, color, { 0.f, 0.f } }  // top-right
			};

			int squareIndices[6] = {
				0, 3, 2, // First triangle
				2, 1, 0  // Second triangle
			};
			Geometry geometry {
				std::span(vertices),
				std::span(squareIndices)
			};

			renderer.draw_geometry(geometry);
		}
		return segmentsRendered;
	}

	void MapElement::render_bounding_box() const {
		// Convert all corners of the bounding box to screen coordinates
		Position topLeft = convert_to_screen({ min_lat, min_lon });
		Position topRight = convert_to_screen({ min_lat, max_lon });
		Position bottomLeft = convert_to_screen({ max_lat, min_lon });
		Position bottomRight = convert_to_screen({ max_lat, max_lon });

		auto& renderer = _application.renderer();

		renderer.set_draw_color({ 1.f, 0.f, 0.f, 1.f });

		renderer.draw_line(topLeft.x, topLeft.y, topRight.x, topRight.y);
		renderer.draw_line(topRight.x, topRight.y, bottomRight.x, bottomRight.y);
		renderer.draw_line(bottomRight.x, bottomRight.y, bottomLeft.x, bottomLeft.y);
		renderer.draw_line(bottomLeft.x, bottomLeft.y, topLeft.x, topLeft.y);
	}

	void MapElement::draw_lane_markers(const std::vector<Position>& nodes, int lanes, int laneWidthPixels) {
		if (_render_data.metersPerPixel > Constants::DRAW_LANE_MARKERS_MPP) {
			return;
		}

		if (nodes.size() < 2) {
			return;
		}

		auto& renderer = _application.renderer();
		renderer.set_draw_color(Constants::LANE_MARKER_COLOR);

		float totalWidth = lanes * Constants::LANE_WIDTH * _render_data.metersPerPixel;
		float laneWidth = totalWidth / lanes;

		for (int lane = 1; lane < lanes; lane++) {
			float offset = -totalWidth / 2 + lane * laneWidth;

			for (size_t i = 0; i < nodes.size() - 1; i++) {
				Position p1 = nodes[i];
				Position p2 = nodes[i + 1];

				// Calculate perpendicular vector
				float dx = p2.x - p1.x;
				float dy = p2.y - p1.y;
				float len = sqrtf(dx * dx + dy * dy);
				if (len == 0) {
					continue;
				}

				float perpx = -dy / len * offset;
				float perpy = dx / len * offset;

				// Draw dashed lane markers
				float segmentLength = 5.0f; // meters
				int segments = static_cast<int>(len / (segmentLength * _render_data.metersPerPixel));

				for (int s = 0; s < segments; s += 2) {
					float t1 = s / static_cast<float>(segments);
					float t2 = (s + 1) / static_cast<float>(segments);

					Position sp1 = {
						static_cast<int>(p1.x + t1 * dx + perpx),
						static_cast<int>(p1.y + t1 * dy + perpy)
					};
					Position sp2 = {
						static_cast<int>(p1.x + t2 * dx + perpx),
						static_cast<int>(p1.y + t2 * dy + perpy)
					};

					renderer.draw_line(sp1.x, sp1.y, sp2.x, sp2.y);

					// Draw direction arrow at the middle of each dashed line
					if (s % 12 == 0) { // Draw arrows less frequently than dashes
						float tmid = (t1 + t2) / 2.0f;
						Position arrow_center = {
							static_cast<int>(p1.x + tmid * dx + perpx),
							static_cast<int>(p1.y + tmid * dy + perpy)
						};

						// Calculate arrow points
						float arrow_size = 3.0f; // pixels
						float arrow_angle = std::atan2(dy, dx);
						float arrow_angle1 = arrow_angle - 0.5f; // 30 degrees
						float arrow_angle2 = arrow_angle + 0.5f; // 30 degrees

						Position arrow_p1 = {
							static_cast<int>(arrow_center.x - arrow_size * std::cos(arrow_angle1)),
							static_cast<int>(arrow_center.y - arrow_size * std::sin(arrow_angle1))
						};
						Position arrow_p2 = {
							static_cast<int>(arrow_center.x - arrow_size * std::cos(arrow_angle2)),
							static_cast<int>(arrow_center.y - arrow_size * std::sin(arrow_angle2))
						};

						// Draw arrow
						renderer.draw_line(arrow_center.x, arrow_center.y, arrow_p1.x, arrow_p1.y);
						renderer.draw_line(arrow_center.x, arrow_center.y, arrow_p2.x, arrow_p2.y);
					}
				}
			}
		}
	}

	void MapElement::draw_path_nodes(const std::vector<Position>& nodes) {
		auto& renderer = _application.renderer();
		renderer.set_draw_color({ 1.0f, 0.0f, 0.0f, 1.0f });

		for (const auto& pos : nodes) {
			renderer.draw_circle(pos.x, pos.y, 3, true);
		}
	}

} // namespace tjs::visualization

#include "stdafx.h"

#include "visualization/elements/map_element.h"

#include "data/persistent_render_data.h"
#include "data/simulation_debug_data.h"

#include <render/render_base.h>
#include <visualization/visualization_constants.h>
#include <data/map_renderer_data.h>
#include <Application.h>
#include <events/map_events.h>

#include <core/data_layer/world_data.h>
#include <core/data_layer/data_types.h>
#include <core/math_constants.h>
#include <core/map_math/path_finder.h>

namespace tjs::visualization {
	using namespace tjs::core;

	bool point_inside_screen(const Position& p, int w, int h) {
		return p.x >= 0 && p.x <= w && p.y >= 0 && p.y <= h;
	}

	bool line_outside_screen(const Position& sp1, const Position& sp2, int w, int h) {
		// Step 1: Check if either endpoint is inside screen
		if (point_inside_screen(sp1, w, h) || point_inside_screen(sp2, w, h)) {
			return false;
		}

		// Step 2: Check for intersection with any screen edge
		// Define screen rectangle as lines
		Position top_left = { 0, 0 };
		Position top_right = { w, 0 };
		Position bottom_left = { 0, h };
		Position bottom_right = { w, h };

		auto intersects = [](Position a1, Position a2, Position b1, Position b2) {
			auto cross = [](Position p1, Position p2) {
				return p1.x * p2.y - p1.y * p2.x;
			};
			Position r = { a2.x - a1.x, a2.y - a1.y };
			Position s = { b2.x - b1.x, b2.y - b1.y };
			Position diff = { b1.x - a1.x, b1.y - a1.y };
			int denom = cross(r, s);
			int num1 = cross(diff, s);
			int num2 = cross(diff, r);
			if (denom == 0) {
				return false; // Parallel
			}
			double t = (double)num1 / denom;
			double u = (double)num2 / denom;
			return t >= 0 && t <= 1 && u >= 0 && u <= 1;
		};

		return !(
			intersects(sp1, sp2, top_left, top_right) || intersects(sp1, sp2, top_right, bottom_right) || intersects(sp1, sp2, bottom_right, bottom_left) || intersects(sp1, sp2, bottom_left, top_left));
	}

	MapElement::MapElement(Application& application)
		: SceneNode("MapElement")
		, _application(application)
		, _render_data(*application.stores().get_model<model::MapRendererData>())
		, _cache(*application.stores().get_model<core::model::PersistentRenderData>())
		, _debugData(*application.stores().get_model<core::model::SimulationDebugData>())
		, _map_positioning(application) {
	}

	MapElement::~MapElement() {
		_application.renderer().unregister_event_listener(&_map_positioning);
	}

	void MapElement::on_map_updated() {
		auto& world = _application.worldData();
		auto& segments = world.segments();

		if (!segments.empty()) {
			auto_zoom(segments.front()->nodes);
		}

		auto& general_settings = _application.settings().general;

		if (_current_file.empty()) {
			_current_file = _application.settings().general.selectedFile;
			_render_data.screen_center = general_settings.screen_center;
			_render_data.metersPerPixel = general_settings.zoomLevel;
		}

		_map_positioning.update_map_positioning();
	}

	void MapElement::init() {
		_application.renderer().register_event_listener(&_map_positioning);
		_application.message_dispatcher().register_handler(*this, &MapElement::handle_open_map_simulation_reinit, "project");
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

		bool draw_network = static_cast<uint32_t>(_render_data.visibleLayers & model::MapRendererLayer::NetworkGraph) != 0;
		// Render all ways if enabled
		if (static_cast<uint32_t>(_render_data.visibleLayers & model::MapRendererLayer::Ways) != 0) {
			const bool draw_nodes = static_cast<uint32_t>(_render_data.visibleLayers & model::MapRendererLayer::Nodes) != 0;

			for (auto& [id, way] : _cache.ways) {
				int nodes_rendered = render_way(way);
				if (!draw_network && draw_nodes) {
					draw_path_nodes(way);
				}
			}
		}

		// Render network graph if enabled
		if (draw_network) {
			if (segment->road_network) {
				render_lanes(renderer, *segment->road_network);
				render_network_graph(renderer, *segment->road_network);
				draw_network_nodes(*segment->road_network);
			}
		}
	}

	void MapElement::render_network_graph(IRenderer& renderer, const core::RoadNetwork& network) {
		// Set color for network graph edges
		renderer.set_draw_color({ 0.0f, 0.8f, 0.8f, 0.5f }); // Semi-transparent cyan

		const auto& nodes = _cache.nodes;
		bool filter = _render_data.networkOnlyForSelected && !_debugData.reachableNodes.empty();

		// Render edges from edge graph
		for (const auto& [node, edges] : network.edge_graph) {
			const bool is_node_filtered = filter && !_debugData.reachableNodes.contains(node->uid);

			auto it = nodes.find(node->uid);
			if (it == nodes.end()) {
				continue;
			}

			const Position& start = it->second.screenPos;
			for (const Edge* edge : edges) {
				Node* neighbor = edge->end_node;
				const bool is_neighbor_filtered = filter && !_debugData.reachableNodes.contains(neighbor->uid);

				auto itNeighbor = nodes.find(neighbor->uid);
				if (itNeighbor == nodes.end()) {
					continue;
				}
				const Position& end = itNeighbor->second.screenPos;
				if (line_outside_screen(start, end, renderer.screen_width(), renderer.screen_height())) {
					continue;
				}

				// Draw edge as a thin line
				const FColor color = (is_node_filtered || is_neighbor_filtered) ? FColor { 0.8f, 0.0f, 0.0f, 0.5f } : FColor { 0.0f, 0.8f, 0.8f, 0.5f };
				drawThickLine(renderer, { start, end }, _render_data.metersPerPixel, 0.8f, color);
			}
		}
	}

	Position convert_to_screen(
		const Coordinates& coord,
		const Position& screen_center,
		double meters_per_pixel) {
		double x = coord.x;
		double y = coord.y;

		// Scale to screen coordinates
		int screenX = static_cast<int>(screen_center.x + x / meters_per_pixel);
		int screenY = static_cast<int>(screen_center.y + y / meters_per_pixel);

		return { screenX, screenY };
	}

	void MapElement::handle_open_map_simulation_reinit(const events::OpenMapEvent& event) {
		on_map_updated();
	}

	Position MapElement::convert_to_screen(const Coordinates& coord) const {
		return tjs::visualization::convert_to_screen(
			coord,
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

		for (const auto& pair : nodes) {
			const auto& node = pair.second;
			double x = node->coordinates.x;
			double y = node->coordinates.y;

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

		double center_x = (minX + maxX) / 2.0;
		double center_y = (minY + maxY) / 2.0;

		_render_data.screen_center.x = static_cast<int>(renderer.screen_width() / 2.0 - center_x / _render_data.metersPerPixel);
		_render_data.screen_center.y = static_cast<int>(renderer.screen_height() / 2.0 - center_y / _render_data.metersPerPixel);
	}

	void MapElement::calculate_map_bounds(const std::unordered_map<uint64_t, std::unique_ptr<Node>>& nodes) {
		// Initialize bounding box with extreme values
		min_lat = std::numeric_limits<float>::max();
		max_lat = std::numeric_limits<float>::lowest();
		min_lon = std::numeric_limits<float>::max();
		max_lon = std::numeric_limits<float>::lowest();
		min_x = std::numeric_limits<double>::max();
		max_x = std::numeric_limits<double>::lowest();
		min_y = std::numeric_limits<double>::max();
		max_y = std::numeric_limits<double>::lowest();

		// Iterate through all nodes to find min/max coordinates
		for (const auto& pair : nodes) {
			const auto& node = pair.second;

			min_lat = std::min(min_lat, static_cast<float>(node->coordinates.latitude));
			max_lat = std::max(max_lat, static_cast<float>(node->coordinates.latitude));
			min_lon = std::min(min_lon, static_cast<float>(node->coordinates.longitude));
			max_lon = std::max(max_lon, static_cast<float>(node->coordinates.longitude));
			min_x = std::min(min_x, node->coordinates.x);
			max_x = std::max(max_x, node->coordinates.x);
			min_y = std::min(min_y, node->coordinates.y);
			max_y = std::max(max_y, node->coordinates.y);
		}
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
				roadColor = Constants::EMERGENCY_COLOR;
				break;
			case WayType::Parking:
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

	int MapElement::render_way(const WayRenderInfo& way) {
		if (way.screenPoints.size() < 2) {
			return 0;
		}

		bool hasVisiblePoints = false;
		std::vector<Position> screenPoints;
		screenPoints.reserve(way.nodes.size());
		for (auto node : way.nodes) {
			screenPoints.emplace_back(node->screenPos);
		}

		const FColor color = get_way_color(way.way->type);

		const float lane_width = way.way->is_car_accessible() ? way.way->lanes * way.way->laneWidth : way.way->laneWidth / 2;

		int segmentsRendered = drawThickLine(_application.renderer(), screenPoints, _render_data.metersPerPixel, lane_width, color);

		if (way.way->lanes > 1) {
			draw_lane_markers(screenPoints, way.way->lanes, Constants::LANE_WIDTH);
		}

		// Draw direction arrows if it's a one-way road or has explicit lane directions
		if (way.way->is_car_accessible() && (way.way->isOneway || (way.way->lanesForward > 0 && way.way->lanesBackward == 0) || (way.way->lanesForward == 0 && way.way->lanesBackward > 0))) {
			draw_direction_arrows(screenPoints, way.way->isOneway && way.way->lanesBackward > 0);
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
		Position topLeft = convert_to_screen(Coordinates { 0.0, 0.0, min_x, min_y });
		Position topRight = convert_to_screen(Coordinates { 0.0, 0.0, max_x, min_y });
		Position bottomLeft = convert_to_screen(Coordinates { 0.0, 0.0, min_x, max_y });
		Position bottomRight = convert_to_screen(Coordinates { 0.0, 0.0, max_x, max_y });

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

				if (line_outside_screen(p1, p2, renderer.screen_width(), renderer.screen_height())) {
					continue;
				}

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
				static const float segmentLength = 5.0f; // meters
				int segments = static_cast<int>(len / segmentLength);
				for (int s = 0; s < segments; s += 3) {
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

					if (line_outside_screen(sp1, sp2, renderer.screen_width(), renderer.screen_height())) {
						continue;
					}

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

	void draw_node(IRenderer& renderer, const NodeRenderInfo& node) {
		const float circle_size = node.selected ? 5.0f : 3.0f;

		if (!point_inside_screen(node.screenPos, renderer.screen_width(), renderer.screen_height())) {
			return;
		}

		renderer.draw_circle(node.screenPos.x, node.screenPos.y, circle_size, true);
	}

	void MapElement::draw_path_nodes(const WayRenderInfo& way) {
		auto& renderer = _application.renderer();
		renderer.set_draw_color({ 1.0f, 0.0f, 0.0f, 1.0f });

		for (const auto node : way.nodes) {
			draw_node(renderer, *node);
		}
	}

	void MapElement::draw_network_nodes(const core::RoadNetwork& network) {
		auto& renderer = _application.renderer();

		bool filter = _render_data.networkOnlyForSelected && !_debugData.reachableNodes.empty();
		for (const auto& [uid, node] : network.nodes) {
			const bool is_filtered = filter && !_debugData.reachableNodes.contains(uid);
			if (auto it = _cache.nodes.find(uid); it != _cache.nodes.end()) {
				const FColor color = is_filtered ? FColor { 1.0f, 0.0f, 0.0f, 1.0f } : FColor { 0.0f, 1.0f, 0.0f, 1.0f };
				renderer.set_draw_color(color);
				draw_node(renderer, it->second);
			}
		}
	}

	static std::vector<const Edge*> edges;
	static Node* selected_prev = nullptr;

	void MapElement::render_lanes(IRenderer& renderer, const core::RoadNetwork& network) {
		auto selected = _debugData.selectedNode;

		if (selected && selected->node != selected_prev) {
			edges.clear();

			for (const auto& edge : network.edges) {
				auto r = core::algo::PathFinder::find_edge_path_a_star(network, selected->node, edge.end_node);
				if (!r.empty()) {
					edges.push_back(&edge);
				}
			}

			selected_prev = selected->node;
		}

		// Render lane centerlines and outgoing connections
		for (const auto& edge : network.edges) {
			if (selected && std::ranges::find(edges, &edge) == edges.end()) {
				continue;
			}

			for (const auto& lane : edge.lanes) {
				// Render lane centerline
				if (lane.centerLine.size() >= 2) {
					std::vector<Position> centerlinePoints;
					centerlinePoints.reserve(lane.centerLine.size());

					for (const auto& coord : lane.centerLine) {
						centerlinePoints.push_back(convert_to_screen(coord));
					}

					// Draw centerline in white
					renderer.set_draw_color({ 1.0f, 1.0f, 1.0f, 0.8f });
					//drawThickLine(renderer, centerlinePoints, _render_data.metersPerPixel, 0.5f, { 1.0f, 1.0f, 1.0f, 0.8f });
				}

				// Render outgoing connections
				for (const auto& outgoing_lane : lane.outgoing_connections) {
					if (outgoing_lane && outgoing_lane->centerLine.size() >= 2) {
						// Check if this connection is bidirectional (in incoming_connections)
						bool is_bidirectional = false;
						for (const auto& incoming_lane : outgoing_lane->incoming_connections) {
							if (incoming_lane == &lane) {
								is_bidirectional = true;
								break;
							}
						}

						// Choose color based on connection type
						FColor connectionColor;
						if (is_bidirectional) {
							connectionColor = { 0.0f, 1.0f, 0.0f, 0.6f }; // Green for bidirectional
						} else {
							connectionColor = { 1.0f, 0.0f, 0.0f, 0.6f }; // Red for unidirectional
						}

						// Draw connection line from end of current lane to start of outgoing lane
						if (!lane.centerLine.empty() && !outgoing_lane->centerLine.empty()) {
							Position start = convert_to_screen(lane.centerLine.front());
							Position end = convert_to_screen(outgoing_lane->centerLine.front());

							// Only draw if both points are visible
							if (!line_outside_screen(start, end, renderer.screen_width(), renderer.screen_height())) {
								renderer.set_draw_color(connectionColor);
								drawThickLine(renderer, { start, end }, _render_data.metersPerPixel, 0.5f, connectionColor);
							}
						}
					}
				}
			}
		}
	}

} // namespace tjs::visualization

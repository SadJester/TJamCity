#include <stdafx.h>

#include <visualization/elements/map_element.h>

#include <data/persistent_render_data.h>
#include <data/map_renderer_data.h>

#include <render/render_base.h>
#include <visualization/visualization_constants.h>

#include <Application.h>
#include <events/map_events.h>

#include <core/data_layer/world_data.h>
#include <core/data_layer/data_types.h>
#include <core/math_constants.h>
#include <core/map_math/path_finder.h>

#include <logic/map/map_positioning.h>

namespace tjs::visualization {
	using namespace tjs::core;

	bool point_inside_screen(const Position& p, int w, int h, int threshold = 0) {
		return p.x >= -threshold && p.x <= (w + threshold) && p.y >= -threshold && p.y <= (h + threshold);
	}

	bool line_outside_screen(const Position& sp1, const Position& sp2, int w, int h, int threshold = 0) {
		// Step 1: Check if either endpoint is inside screen
		if (point_inside_screen(sp1, w, h, threshold) || point_inside_screen(sp2, w, h, threshold)) {
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
		, _render_data(*application.stores().get_entry<model::MapRendererData>())
		, _cache(*application.stores().get_entry<core::model::PersistentRenderData>())
		, _debugData(application.stores().get_entry<core::simulation::SimulationDebugData>()) {
	}

	MapElement::~MapElement() {
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

		auto positioning = _application.logic_modules().get_entry<app::logic::MapPositioning>();
		if (positioning) {
			positioning->update_map_positioning();
		}
	}

	void MapElement::init() {
		_application.message_dispatcher().register_handler(*this, &MapElement::handle_open_map_simulation_reinit, "project");
	}

	void MapElement::update() {
	}

	FColor get_way_color(WayType type) {
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

	Position normalize(const Coordinates& v) {
		double len = std::sqrt(v.x * v.x + v.y * v.y);
		return len > 0 ? Position { static_cast<int>(v.x / len), static_cast<int>(v.y / len) } : Position { 0, 0 };
	}

	Position operator*(const Position& pos, double multiplier) {
		return Position { static_cast<int>(pos.x * multiplier), static_cast<int>(pos.y * multiplier) };
	}

	Position operator+(const Position& a, const Position& b) {
		return Position { a.x + b.x, a.y + b.y };
	}

	Position operator-(const Position& a, const Position& b) {
		return Position { a.x - b.x, a.y - b.y };
	}

	FPoint normalize_f(const Coordinates& v) {
		double len = std::sqrt(v.x * v.x + v.y * v.y);
		return len > 0 ? FPoint { static_cast<float>(v.x / len), static_cast<float>(v.y / len) } : FPoint { 0, 0 };
	}

	FPoint operator*(const FPoint& pos, double multiplier) {
		return FPoint { static_cast<float>(pos.x * multiplier), static_cast<float>(pos.y * multiplier) };
	}

	FPoint operator+(const FPoint& a, const FPoint& b) {
		return FPoint { a.x + b.x, a.y + b.y };
	}

	FPoint operator-(const FPoint& a, const FPoint& b) {
		return FPoint { a.x - b.x, a.y - b.y };
	}

	FPoint convert_to_screen_f(const Coordinates& coord, const Position& screen_center, double mpp) {
		float screenX = static_cast<float>(screen_center.x) + static_cast<float>(coord.x / mpp);
		float screenY = static_cast<float>(screen_center.y) - static_cast<float>(coord.y / mpp);
		return { screenX, screenY };
	}

	void draw_dashed_line(IRenderer& renderer,
		const FPoint& start,
		const FPoint& end,
		double metersPerPixel,
		float thickness,
		FColor color,
		float dash_m = 3.0f,
		float gap_m = 3.0f) {
		double dx = end.x - start.x;
		double dy = end.y - start.y;
		double dist = std::sqrt(dx * dx + dy * dy);
		if (dist < 1e-3) {
			return;
		}

		float dash_px = dash_m / metersPerPixel;
		float gap_px = gap_m / metersPerPixel;
		double dir_x = dx / dist;
		double dir_y = dy / dist;
		double progress = 0.0;
		while (progress < dist) {
			double seg_end = std::min(dist, progress + dash_px);
			FPoint p1 { static_cast<float>(start.x + dir_x * progress), static_cast<float>(start.y + dir_y * progress) };
			FPoint p2 { static_cast<float>(start.x + dir_x * seg_end), static_cast<float>(start.y + dir_y * seg_end) };
			drawThickLine(renderer, { p1, p2 }, metersPerPixel, thickness, color);
			progress += dash_px + gap_px;
		}
	}

	int drawThickLine(IRenderer& renderer, const std::vector<FPoint>& nodes, double metersPerPixel, float thickness, FColor color) {
		if (nodes.size() < 2) {
			return 0;
		}

		thickness /= metersPerPixel;

		int segmentsRendered = 0;
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

	struct LaneDirectionRenderer {
		static void render_lane_arrow(IRenderer& renderer, const Lane& lane, double mpp, const Position& screen_center, FColor color) {
			if (lane.centerLine.size() < 2) {
				return;
			}
			// 1. Get last segment of lane and convert to screen coordinates
			const Coordinates& tail_world = lane.centerLine[lane.centerLine.size() - 2];
			const Coordinates& tip_world = lane.centerLine.back();

			float arrow_offset_px = 3.0f / mpp;
			float min_lane_px = 15.0f / mpp;
			FPoint p_tail = convert_to_screen_f(tail_world, screen_center, mpp);
			FPoint p_tip = convert_to_screen_f(tip_world, screen_center, mpp);

			// 2. Direction and perpendicular (screen space)
			FPoint dir = p_tip - p_tail;
			float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
			if (len < 1e-3f || len < min_lane_px) {
				return;
			}
			dir.x /= len;
			dir.y /= len;

			// move arrow not to the end
			p_tip = p_tip - dir * arrow_offset_px;

			FPoint perp = { -dir.y, dir.x }; // screen-space perpendicular
			// 3. Arrow size in pixels
			float shaft_length_m = 5.0f;
			float shaft_offset_m = 3.0f;
			float shaft_width_m = 0.2f;

			// 4. Make base
			float shaft_length_px = shaft_length_m / mpp;
			float shaft_offset_px = shaft_offset_m / mpp;
			float shaft_half_width_px = (shaft_width_m / mpp) * 0.5f;

			FPoint shaft_end = p_tip - dir * shaft_offset_px; // Ends 3m before tip
			std::vector<Vertex> vertices;
			std::vector<int> indices;

			// Most of lanes will be base + arrow (7 points and 3 trigs (9 indices))
			vertices.reserve(7);
			indices.reserve(9);

			add_base(vertices, indices, shaft_end, dir, shaft_length_px, shaft_width_m / mpp, color);
			// 5. Build arrow geometry
			add_lane_direction(lane, vertices, indices, shaft_end, color, mpp, dir, perp);

			Geometry geometry {
				std::span(vertices.data(), vertices.size()),
				std::span(indices.data(), indices.size())
			};

			renderer.draw_geometry(geometry);
		}

		static void add_base(std::vector<Vertex>& _vert, std::vector<int>& _ind, const FPoint& start, const FPoint& dir, float length, float width, const FColor& color) {
			const int vert_size = _vert.size();
			FPoint shaft_end = start; // Ends 3m before tip
			FPoint shaft_start = start - dir * length;

			FPoint perp = { -dir.y, dir.x };

			float half_w = width / 2;
			FPoint shaft_left0 = shaft_start - perp * half_w;
			FPoint shaft_right0 = shaft_start + perp * half_w;
			FPoint shaft_left1 = shaft_end - perp * half_w;
			FPoint shaft_right1 = shaft_end + perp * half_w;

			_vert.push_back({ shaft_left0, color });  // 0
			_vert.push_back({ shaft_right0, color }); // 1
			_vert.push_back({ shaft_right1, color }); // 2
			_vert.push_back({ shaft_left1, color });  // 3

			_ind.push_back(vert_size + 0);
			_ind.push_back(vert_size + 1);
			_ind.push_back(vert_size + 2);
			_ind.push_back(vert_size + 0);
			_ind.push_back(vert_size + 2);
			_ind.push_back(vert_size + 3);
		}

		static void add_arrow(std::vector<Vertex>& _vert, std::vector<int>& _ind, const FPoint& arrow_start, const FColor& color, float mpp, const FPoint& dir, const FPoint& perp) {
			const size_t vert_size = _vert.size();
			const float arrow_length_m = 0.5f;
			const float arrow_width_m = 0.35f;
			const float arrow_length_px = arrow_length_m / mpp;
			const float arrow_half_width_px = (arrow_width_m / mpp) * 0.5f;
			const float turn_arrow_offset_px = 1.5f / mpp;

			FPoint head_center = arrow_start + dir * arrow_length_px;
			FPoint head_left = arrow_start - perp * arrow_half_width_px;
			FPoint head_right = arrow_start + perp * arrow_half_width_px;

			_vert.push_back({ head_left, color });   // 0
			_vert.push_back({ head_center, color }); // 1
			_vert.push_back({ head_right, color });  // 2

			_ind.push_back(vert_size + 0);
			_ind.push_back(vert_size + 1);
			_ind.push_back(vert_size + 2);
		}

		static void add_lane_direction(const Lane& lane, std::vector<Vertex>& _vert, std::vector<int>& _ind, const FPoint& arrow_start, const FColor& color, float mpp, const FPoint& dir, const FPoint& perp) {
			const auto turn_direction = lane.turn == TurnDirection::None ? TurnDirection::Straight : lane.turn;
			const float arrow_length_m = 0.5f;
			const float arrow_width_m = 0.35f;
			const float arrow_length_px = arrow_length_m / mpp;
			const float arrow_half_width_px = (arrow_width_m / mpp) * 0.5f;
			const float turn_arrow_offset_px = 1.5f / mpp;
			const float shaft_width_px = 0.2f / mpp;

			if (has_flag(turn_direction, TurnDirection::Straight)) {
				add_arrow(_vert, _ind, arrow_start, color, mpp, dir, perp);
			}

			if (has_flag(turn_direction, TurnDirection::Left)) {
				// Start position offset to the left
				FPoint turn_start = arrow_start - dir * turn_arrow_offset_px;
				FPoint turn_dir = perp; // left direction

				add_base(_vert, _ind, turn_start, turn_dir, arrow_length_px, shaft_width_px, color);

				FPoint turn_tip = turn_start + turn_dir * arrow_length_px;
				add_arrow(_vert, _ind, turn_tip, color, mpp, turn_dir, dir); // arrow points left, use `dir` as new perp
			}

			if (has_flag(turn_direction, TurnDirection::Right)) {
				// Start position offset to the right
				FPoint turn_start = arrow_start - dir * turn_arrow_offset_px;
				FPoint turn_dir = perp * -1.0f; // right direction
				add_base(_vert, _ind, turn_start, turn_dir, arrow_length_px, shaft_width_px, color);

				FPoint turn_tip = turn_start - turn_dir * arrow_length_px;
				add_arrow(_vert, _ind, turn_tip, color, mpp, perp, dir); // arrow points right, use `-dir` as new perp
			}
		}
	};

	void draw_diamond(IRenderer& renderer, const Position& center, float size, FColor color) {
		std::vector<Vertex> vertices;
		std::vector<int> indices;

		const float half_size = size * 0.5f;

		FPoint left = { center.x - half_size, static_cast<float>(center.y) };
		FPoint right = { center.x + half_size, static_cast<float>(center.y) };
		FPoint top = { static_cast<float>(center.x), center.y - half_size };
		FPoint bottom = { static_cast<float>(center.x), center.y + half_size };

		vertices.push_back({ left, color });   // 0
		vertices.push_back({ right, color });  // 1
		vertices.push_back({ top, color });    // 2
		vertices.push_back({ bottom, color }); // 3

		// Two triangles to make a diamond
		indices = {
			0, 2, 1, // top triangle
			0, 1, 3  // bottom triangle
		};

		Geometry geometry {
			std::span(vertices.data(), vertices.size()),
			std::span(indices.data(), indices.size())
		};

		renderer.draw_geometry(geometry);
	}

	void draw_node(IRenderer& renderer, Node& node, bool is_selected, const Position& screen_center, double mpp) {
		const float circle_size = is_selected ? 20.0f : 15.0f;
		auto position = convert_to_screen(node.coordinates, screen_center, mpp);

		if (!point_inside_screen(position, renderer.screen_width(), renderer.screen_height())) {
			return;
		}

		draw_diamond(renderer, position, circle_size, FColor::Red);
	}

	void render_network(IRenderer& renderer, const WorldSegment& segment, core::model::MapRendererData& render_data, core::simulation::SimulationDebugData* debug_data) {
		const bool render_nodes = static_cast<uint32_t>(render_data.visibleLayers & model::MapRendererLayer::Nodes) != 0;
		auto& screen_center = render_data.screen_center;
		double mpp = render_data.metersPerPixel;
		core::Node* selected_node = debug_data != nullptr ? debug_data->selectedNode : nullptr;
		const bool simplified = render_data.metersPerPixel > render_data.simplifiedViewThreshold;

		enum class LaneType {
			None,
			Outgoing,
			Incoming,
			Selected
		};

		auto _render_lane = [&renderer, &screen_center, mpp](const Lane& lane, const FColor& color, float thickness, LaneType lane_type) {
			FColor altered_color;
			float debug_thickness = 0.2f;

			if (lane_type == LaneType::Incoming) {
				altered_color = Constants::INCOMING_COLOR;
				debug_thickness = Constants::DEBUG_INCOMING_LANE_THICKNESS;
			} else if (lane_type == LaneType::Outgoing) {
				altered_color = Constants::OUTGOING_COLOR;
				debug_thickness = Constants::DEBUG_OUTGOING_LANE_THICKNESS;
			} else if (lane_type == LaneType::Selected) {
				altered_color = FColor::Blue;
			}

			FPoint start = convert_to_screen_f(lane.centerLine.front(), screen_center, mpp);
			FPoint end = convert_to_screen_f(lane.centerLine.back(), screen_center, mpp);

			Position is_start { static_cast<int>(start.x), static_cast<int>(start.y) };
			Position is_end { static_cast<int>(end.x), static_cast<int>(end.y) };

			// Only draw if both points are visible
			if (!line_outside_screen(is_start, is_end, renderer.screen_width(), renderer.screen_height(), (thickness / mpp) * 2)) {
				drawThickLine(renderer, { start, end }, mpp, thickness, color);
				LaneDirectionRenderer::render_lane_arrow(renderer, lane, mpp, screen_center, Constants::ARROW_COLOR);

				if (lane_type != LaneType::None) {
					drawThickLine(renderer, { start, end }, mpp, debug_thickness, altered_color);
				}
			}
		};

		const Node* selected = selected_node;
		const auto& ways = segment.sorted_ways;

		const bool filter = render_data.networkOnlyForSelected && debug_data != nullptr && !debug_data->reachableNodes.empty();

		std::unordered_set<const Lane*> outgoing_highlight;
		std::unordered_set<const Lane*> incoming_highlight;
		if (render_data.selected_lane) {
			for (const auto& link : render_data.selected_lane->incoming_connections) {
				if (link->from) {
					incoming_highlight.insert(link->from);
				}
			}
			for (const auto& link : render_data.selected_lane->outgoing_connections) {
				if (link->to) {
					outgoing_highlight.insert(link->to);
				}
			}
		}

		if (simplified) {
			for (const WayInfo* way : ways) {
				auto color = get_way_color(way->type);
				float thickness = static_cast<float>(way->laneWidth * way->lanes);
				for (auto edge : way->edges) {
					FPoint start = convert_to_screen_f(edge->start_node->coordinates, screen_center, mpp);
					FPoint end = convert_to_screen_f(edge->end_node->coordinates, screen_center, mpp);
					Position is_start { static_cast<int>(start.x), static_cast<int>(start.y) };
					Position is_end { static_cast<int>(end.x), static_cast<int>(end.y) };
					if (!line_outside_screen(is_start, is_end, renderer.screen_width(), renderer.screen_height(), (thickness / mpp) * 2)) {
						drawThickLine(renderer, { start, end }, mpp, thickness, color);
					}
				}
			}

			if (render_nodes && mpp < Constants::DRAW_LANE_DETAILS_MPP) {
				for (auto& [_, node] : segment.nodes) {
					if (!node->hasTag(NodeTags::Way)) {
						continue;
					}
					const bool is_filtered = filter && !debug_data->reachableNodes.contains(node->uid);
					if (!is_filtered) {
						draw_node(renderer, *node, node.get() == selected, screen_center, mpp);
					}
				}
			}
			return;
		}

		for (const WayInfo* way : ways) {
			auto color = get_way_color(way->type);
			for (auto edge : way->edges) {
				for (const auto& lane : edge->lanes) {
					LaneType lane_type { LaneType::None };
					if (edge->end_node == selected) {
						lane_type = LaneType::Incoming;
					} else if (edge->start_node == selected) {
						lane_type = LaneType::Outgoing;
					}

					if (outgoing_highlight.contains(&lane)) {
						lane_type = LaneType::Outgoing;
					} else if (incoming_highlight.contains(&lane)) {
						lane_type = LaneType::Incoming;
					} else if (&lane == render_data.selected_lane) {
						lane_type = LaneType::Selected;
					}

					_render_lane(lane, color, way->laneWidth, lane_type);
				}

				if (edge->lanes.size() > 1) {
					for (size_t i = 1; i < edge->lanes.size(); ++i) {
						const auto& l0 = edge->lanes[i - 1];
						const auto& l1 = edge->lanes[i];
						Coordinates start_world {
							0.0,
							0.0,
							(l0.centerLine.front().x + l1.centerLine.front().x) * 0.5,
							(l0.centerLine.front().y + l1.centerLine.front().y) * 0.5
						};
						Coordinates end_world {
							0.0,
							0.0,
							(l0.centerLine.back().x + l1.centerLine.back().x) * 0.5,
							(l0.centerLine.back().y + l1.centerLine.back().y) * 0.5
						};
						FPoint start = convert_to_screen_f(start_world, screen_center, mpp);
						FPoint end = convert_to_screen_f(end_world, screen_center, mpp);
						Position is_start { static_cast<int>(start.x), static_cast<int>(start.y) };
						Position is_end { static_cast<int>(end.x), static_cast<int>(end.y) };
						if (!line_outside_screen(is_start, is_end, renderer.screen_width(), renderer.screen_height())) {
							draw_dashed_line(
								renderer,
								start,
								end,
								mpp,
								Constants::DIVIDING_STRIP_WIDTH,
								Constants::LANE_MARKER_COLOR);
						}
					}
				}

				if (edge->orientation == LaneOrientation::Forward && way->lanesBackward > 0) {
					Edge* opposite = nullptr;
					for (auto other : way->edges) {
						if (other->orientation == LaneOrientation::Backward
							&& other->start_node == edge->end_node
							&& other->end_node == edge->start_node) {
							opposite = &(*other);
							break;
						}
					}
					if (opposite) {
						const auto& lf = edge->opposite_side == Edge::OppositeSide::Right ? edge->lanes.front() : edge->lanes.back();
						const auto& lb = opposite->opposite_side == Edge::OppositeSide::Right ? opposite->lanes.front() : opposite->lanes.back();
						Coordinates start_world {
							0.0,
							0.0,
							(lf.centerLine.front().x + lb.centerLine.back().x) * 0.5,
							(lf.centerLine.front().y + lb.centerLine.back().y) * 0.5
						};
						Coordinates end_world {
							0.0,
							0.0,
							(lf.centerLine.back().x + lb.centerLine.front().x) * 0.5,
							(lf.centerLine.back().y + lb.centerLine.front().y) * 0.5
						};
						FPoint start = convert_to_screen_f(start_world, screen_center, mpp);
						FPoint end = convert_to_screen_f(end_world, screen_center, mpp);
						Position is_start { static_cast<int>(start.x), static_cast<int>(start.y) };
						Position is_end { static_cast<int>(end.x), static_cast<int>(end.y) };
						if (!line_outside_screen(is_start, is_end, renderer.screen_width(), renderer.screen_height())) {
							drawThickLine(
								renderer,
								{ start, end },
								mpp,
								Constants::DOUBLE_SOLID_STRIP_WIDTH,
								Constants::LANE_MARKER_COLOR);
						}
					}
				}
			}
		}

		// There is no need to render nodes when the zoom is too high
		if (render_nodes && mpp < Constants::DRAW_LANE_DETAILS_MPP) {
			for (auto& [_, node] : segment.nodes) {
				if (!node->hasTag(NodeTags::Way)) {
					continue;
				}
				const bool is_filtered = filter && !debug_data->reachableNodes.contains(node->uid);
				if (!is_filtered) {
					draw_node(renderer, *node, node.get() == selected, screen_center, mpp);
				}
			}
		}
	}

	void MapElement::render(IRenderer& renderer) {
		TJS_TRACY_NAMED("MapElement_Render");
		auto& world = _application.worldData();
		auto& segments = world.segments();

		if (segments.empty()) {
			return;
		}

		auto& segment = segments.front();

		if (_render_data.showBoundingBox) {
			render_bounding_box();
		}

		render_network(renderer, *segment, _render_data, _debugData);

		bool draw_network = static_cast<uint32_t>(_render_data.visibleLayers & model::MapRendererLayer::NetworkGraph) != 0;
		// Render network graph if enabled
		if (draw_network) {
			if (segment->road_network) {
				render_network_graph(renderer, *segment->road_network);
			}
		}
	}

	void MapElement::render_network_graph(IRenderer& renderer, const core::RoadNetwork& network) {
		TJS_TRACY_NAMED("MapElement_Render_Graph");
		// Set color for network graph edges
		renderer.set_draw_color({ 0.0f, 0.8f, 0.8f, 0.5f }); // Semi-transparent cyan

		bool filter = _render_data.networkOnlyForSelected && _debugData != nullptr && !_debugData->reachableNodes.empty();
		// Render edges from edge graph
		for (const auto& [node, edges] : network.edge_graph) {
			const bool is_node_filtered = filter && !_debugData->reachableNodes.contains(node->uid);
			const FPoint start = convert_to_screen_f(node->coordinates, _render_data.screen_center, _render_data.metersPerPixel);
			for (const Edge* edge : edges) {
				Node* neighbor = edge->end_node;
				const bool is_neighbor_filtered = filter && !_debugData->reachableNodes.contains(neighbor->uid);
				const FPoint end = convert_to_screen_f(neighbor->coordinates, _render_data.screen_center, _render_data.metersPerPixel);
				Position is_start { static_cast<int>(start.x), static_cast<int>(start.y) };
				Position is_end { static_cast<int>(end.x), static_cast<int>(end.y) };
				if (line_outside_screen(is_start, is_end, renderer.screen_width(), renderer.screen_height())) {
					continue;
				}

				// Draw edge as a thin line
				const FColor color = (is_node_filtered || is_neighbor_filtered) ? FColor { 0.8f, 0.0f, 0.0f, 0.5f } : FColor { 0.0f, 0.8f, 0.8f, 0.5f };
				drawThickLine(renderer, { start, end }, _render_data.metersPerPixel, 1.0f, color);
			}
		}
	}

	Position convert_to_screen(
		const Coordinates& coord,
		const Position& screen_center,
		double meters_per_pixel) {
		double x = coord.x;
		double y = -coord.y;

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

} // namespace tjs::visualization

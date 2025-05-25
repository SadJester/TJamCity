#include "stdafx.h"

#include "visualization/elements/map_element.h"

#include "render/render_base.h"
#include "visualization/visualization_constants.h"
#include "Application.h"

#include <core/data_layer/world_data.h>
#include <core/data_layer/data_types.h>

namespace tjs::visualization {
	using namespace tjs::core;

	MapElement::MapElement(Application& application)
		: SceneNode("MapElement")
		, _application(application) {
	}

	void MapElement::init() {
		auto& world = _application.worldData();
		auto& segments = world.segments();

		if (!segments.empty()) {
			autoZoom(segments.front()->nodes);
		}
	}

	void MapElement::setProjectionCenter(const Coordinates& center) {
		projectionCenter = center;
	}

	void MapElement::setZoomLevel(double metersPerPixel) {
		this->metersPerPixel = metersPerPixel;
		double latRad = projectionCenter.latitude * Constants::DEG_TO_RAD;
		metersPerPixel *= std::cos(latRad);
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

		renderBoundingBox();
		// Render all ways
		int waysRendered = 0;
		int totalNodesRendered = 0;
		int realNodeSegments = 0;
		for (const auto& wayPair : segment->ways) {
			int nodesRendered = renderWay(*wayPair.second, segment->nodes);
			realNodeSegments += wayPair.second->nodes.size() / 2;
			if (nodesRendered > 0) {
				waysRendered++;
			}
			totalNodesRendered += nodesRendered;
		}
	}

	void MapElement::setView(const Coordinates& center, double zoomMetersPerPixel) {
		projectionCenter = center;
		setZoomLevel(zoomMetersPerPixel);
	}

	Position MapElement::convertToScreen(const Coordinates& coord) const {
		// Convert geographic coordinates to meters using Mercator projection
		double x = (coord.longitude - projectionCenter.longitude) * Constants::DEG_TO_RAD * Constants::EARTH_RADIUS;
		double y = -std::log(std::tan((90.0 + coord.latitude) * Constants::DEG_TO_RAD / 2.0)) * Constants::EARTH_RADIUS;
		double yCenter = -std::log(std::tan((90.0 + projectionCenter.latitude) * Constants::DEG_TO_RAD / 2.0)) * Constants::EARTH_RADIUS;
		y -= yCenter;

		// Scale to screen coordinates
		int screenX = static_cast<int>(screenCenterX + x / metersPerPixel);
		int screenY = static_cast<int>(screenCenterY + y / metersPerPixel);

		return { screenX, screenY };
	}

	void MapElement::autoZoom(const std::unordered_map<uint64_t, std::unique_ptr<Node>>& nodes) {
		if (nodes.empty()) {
			return;
		}

		calculateMapBounds(nodes);

		//projectionCenter.latitude = 45.117755;
		//projectionCenter.longitude = 38.981595;

		// Calculate bounds in meters
		double minX = std::numeric_limits<double>::max();
		double maxX = std::numeric_limits<double>::lowest();
		double minY = std::numeric_limits<double>::max();
		double maxY = std::numeric_limits<double>::lowest();

		double yCenter = -std::log(std::tan((90.0 + projectionCenter.latitude) * Constants::DEG_TO_RAD / 2.0)) * Constants::EARTH_RADIUS;

		for (const auto& pair : nodes) {
			const auto& node = pair.second;
			double x = (node->coordinates.longitude - projectionCenter.longitude) * Constants::DEG_TO_RAD * Constants::EARTH_RADIUS;
			double y = -std::log(std::tan((90.0 + node->coordinates.latitude) * Constants::DEG_TO_RAD / 2.0)) * Constants::EARTH_RADIUS - yCenter;

			minX = std::min(minX, x);
			maxX = std::max(maxX, x);
			minY = std::min(minY, y);
			maxY = std::max(maxY, y);
		}

		// Calculate required zoom level
		double widthMeters = maxX - minX;
		double heightMeters = maxY - minY;

		auto& renderer = _application.renderer();

		double zoomX = widthMeters / (renderer.screenWidth() * 0.9);
		double zoomY = heightMeters / (renderer.screenHeight() * 0.9);

		setZoomLevel(std::min(zoomX, zoomY)); // Use min to ensure the entire map fits

		// Recalculate screen center based on the new zoom level
		screenCenterX = renderer.screenWidth() / 2.0;
		screenCenterY = renderer.screenHeight() / 2.0;
	}

	void MapElement::calculateMapBounds(const std::unordered_map<uint64_t, std::unique_ptr<Node>>& nodes) {
		// Initialize bounding box with extreme values
		minLat = std::numeric_limits<float>::max();
		maxLat = std::numeric_limits<float>::lowest();
		minLon = std::numeric_limits<float>::max();
		maxLon = std::numeric_limits<float>::lowest();

		// Iterate through all nodes to find min/max coordinates
		for (const auto& pair : nodes) {
			const auto& node = pair.second;

			minLat = std::min(minLat, static_cast<float>(node->coordinates.latitude));
			maxLat = std::max(maxLat, static_cast<float>(node->coordinates.latitude));
			minLon = std::min(minLon, static_cast<float>(node->coordinates.longitude));
			maxLon = std::max(maxLon, static_cast<float>(node->coordinates.longitude));
		}

		// Calculate the center of the bounding box
		projectionCenter.latitude = (minLat + maxLat) / 2.0f;
		projectionCenter.longitude = (minLon + maxLon) / 2.0f;
	}

	FColor MapElement::getWayColor(WayTags tags) const {
		FColor roadColor = Constants::ROAD_COLOR;
		if (hasFlag(tags, WayTags::Motorway)) {
			roadColor = Constants::MOTORWAY_COLOR;
		} else if (hasFlag(tags, WayTags::Primary)) {
			roadColor = Constants::PRIMARY_COLOR;
		} else if (hasFlag(tags, WayTags::Residential)) {
			roadColor = Constants::RESIDENTIAL_COLOR;
		}
		return roadColor;
	}

	int MapElement::renderWay(const WayInfo& way, const std::unordered_map<uint64_t, std::unique_ptr<Node>>& nodes) {
		if (way.nodeRefs.size() < 2) {
			return 0;
		}

		std::vector<Position> screenPoints;
		screenPoints.reserve(way.nodes.size());
		for (Node* node : way.nodes) {
			screenPoints.push_back(convertToScreen(node->coordinates));
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

		const FColor color = getWayColor(way.tags);
		int segmentsRendered = drawThickLine(screenPoints, way.lanes * Constants::LANE_WIDTH, color);

		if (way.lanes > 1) {
			drawLaneMarkers(screenPoints, way.lanes, Constants::LANE_WIDTH);
		}

		return segmentsRendered;
	}

	void MapElement::renderBoundingBox() const {
		// Convert all corners of the bounding box to screen coordinates
		Position topLeft = convertToScreen({ minLat, minLon });
		Position topRight = convertToScreen({ minLat, maxLon });
		Position bottomLeft = convertToScreen({ maxLat, minLon });
		Position bottomRight = convertToScreen({ maxLat, maxLon });

		auto& renderer = _application.renderer();

		renderer.setDrawColor({ 1.f, 0.f, 0.f, 1.f });

		renderer.drawLine(topLeft.x, topLeft.y, topRight.x, topRight.y);
		renderer.drawLine(topRight.x, topRight.y, bottomRight.x, bottomRight.y);
		renderer.drawLine(bottomRight.x, bottomRight.y, bottomLeft.x, bottomLeft.y);
		renderer.drawLine(bottomLeft.x, bottomLeft.y, topLeft.x, topLeft.y);
	}

	int MapElement::drawThickLine(const std::vector<Position>& nodes, float thickness, FColor color) {
		if (nodes.size() < 2) {
			return 0;
		}

		auto& renderer = _application.renderer();

		thickness /= metersPerPixel;

		int segmentsRendered = 0;
		renderer.setDrawColor(color);

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

			renderer.drawGeometry(geometry);
		}
		return segmentsRendered;
	}

	void MapElement::drawLaneMarkers(const std::vector<Position>& nodes, int lanes, int laneWidthPixels) {
		if (metersPerPixel > Constants::DRAW_LANE_MARKERS_MPP) {
			return;
		}

		if (nodes.size() < 2) {
			return;
		}

		auto& renderer = _application.renderer();
		renderer.setDrawColor(Constants::LANE_MARKER_COLOR);

		float totalWidth = lanes * Constants::LANE_WIDTH * metersPerPixel;
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
				int segments = static_cast<int>(len / (segmentLength * metersPerPixel));

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

					renderer.drawLine(sp1.x, sp1.y, sp2.x, sp2.y);
				}
			}
		}
	}

} // namespace tjs::visualization

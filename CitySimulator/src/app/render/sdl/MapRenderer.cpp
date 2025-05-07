#include "stdafx.h"

#include "render/sdl/MapRenderer.h"

#include "render/RenderBase.h"
#include "render/sdl/SDLRenderer.h"
#include "render/sdl/RenderConstants.h"
#include "Application.h"


namespace tjs::render::visualization {
    using namespace tjs::core;
    
    MapRenderer::MapRenderer(Application& application, SDL_Renderer* sdlRenderer)
        : _application(application)
        , renderer(sdlRenderer) {

    }
    
    void MapRenderer::setView(const Coordinates& center, double zoomMetersPerPixel) {
        projectionCenter = center;
        metersPerPixel = zoomMetersPerPixel;
        
        // Adjust for Mercator projection stretching at high latitudes
        double latRad = center.latitude * Constants::DEG_TO_RAD;
        metersPerPixel *= std::cos(latRad);
    }
    
    SDL_Point MapRenderer::convertToScreen(const Coordinates& coord) const {
        // Convert geographic coordinates to meters using Mercator projection
        double x = (coord.longitude - projectionCenter.longitude) * Constants::DEG_TO_RAD * Constants::EARTH_RADIUS;
        double y = -std::log(std::tan((90.0 + coord.latitude) * Constants::DEG_TO_RAD / 2.0)) * Constants::EARTH_RADIUS;
        double yCenter = -std::log(std::tan((90.0 + projectionCenter.latitude) * Constants::DEG_TO_RAD / 2.0)) * Constants::EARTH_RADIUS;
        y -= yCenter;
        
        // Scale to screen coordinates
        int screenX = static_cast<int>(screenCenterX + x / metersPerPixel);
        int screenY = static_cast<int>(screenCenterY + y / metersPerPixel);
        
        return {screenX, screenY};
    }
    
    void MapRenderer::autoZoom(const std::unordered_map<uint64_t, std::unique_ptr<Node>>& nodes) {
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
    
        metersPerPixel = std::min(zoomX, zoomY); // Use min to ensure the entire map fits
    
        // Recalculate screen center based on the new zoom level
        screenCenterX = renderer.screenWidth() / 2.0;
        screenCenterY = renderer.screenHeight() / 2.0;
    
        // Adjust for latitude
        double latRad = projectionCenter.latitude * Constants::DEG_TO_RAD;
        metersPerPixel *= std::cos(latRad);
    }

    void MapRenderer::calculateMapBounds(const std::unordered_map<uint64_t, std::unique_ptr<Node>>& nodes) {
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

    SDL_FColor MapRenderer::getWayColor(WayTags tags) const {
        SDL_FColor roadColor = Constants::ROAD_COLOR;
        if (hasFlag(tags, WayTags::Motorway)) {
            roadColor = Constants::MOTORWAY_COLOR;
        }
        else if (hasFlag(tags, WayTags::Primary)) {
            roadColor = Constants::PRIMARY_COLOR;
        }
        else if (hasFlag(tags, WayTags::Residential)) {
            roadColor = Constants::RESIDENTIAL_COLOR;
        }
        return roadColor;
    }
    
    int MapRenderer::renderWay(const WayInfo& way, const std::unordered_map<uint64_t, std::unique_ptr<Node>>& nodes) {
        if (way.nodeRefs.size() < 2) { 
            return 0;
        }
        
        std::vector<SDL_Point> screenPoints;
        bool hasOut = false;
        for (Node* node : way.nodes) {
            if (38.97230 > node->coordinates.longitude || 38.99089 < node->coordinates.longitude) {
                hasOut = true;
            }

            if (node->coordinates.latitude > 45.12687 || node->coordinates.latitude < 45.10864) {
                hasOut = true;
            }

            screenPoints.push_back(convertToScreen(node->coordinates));
        }
        
        // Draw the way
        if (screenPoints.size() < 2)  {
            return 0;
        }

        SDL_FColor color = getWayColor(way.tags);
        color = hasOut ? SDL_FColor{1.0f, 0.0f, 0.0f, 1.0f} : SDL_FColor{0.f, 1.f, 0.f, 1.f};
        int segmentsRendered = drawThickLine(screenPoints, way.lanes * Constants::LANE_WIDTH, color);
        
        if (way.lanes > 1) {
            drawLaneMarkers(screenPoints, way.lanes, Constants::LANE_WIDTH);
        }

        return segmentsRendered;
    }
    
    void MapRenderer::renderBoundingBox() const {
        // Convert all corners of the bounding box to screen coordinates
        SDL_Point topLeft = convertToScreen({minLat, minLon});
        SDL_Point topRight = convertToScreen({minLat, maxLon});
        SDL_Point bottomLeft = convertToScreen({maxLat, minLon});
        SDL_Point bottomRight = convertToScreen({maxLat, maxLon});
        
        // Set the color for the bounding box (red)
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        
        // Draw the bounding box lines
        SDL_RenderLine(renderer, topLeft.x, topLeft.y, topRight.x, topRight.y);
        SDL_RenderLine(renderer, topRight.x, topRight.y, bottomRight.x, bottomRight.y);
        SDL_RenderLine(renderer, bottomRight.x, bottomRight.y, bottomLeft.x, bottomLeft.y);
        SDL_RenderLine(renderer, bottomLeft.x, bottomLeft.y, topLeft.x, topLeft.y);
    }

    int MapRenderer::drawThickLine(const std::vector<SDL_Point>& nodes, float thickness, SDL_FColor color) {
        if (nodes.size() < 2) { 
            return 0;
        }
        
        int segmentsRendered = 0;
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        
        for (size_t i = 0; i < nodes.size() - 1; i++) {
            auto p1 = nodes[i];
            auto p2 = nodes[i+1];
            
            // Calculate perpendicular vector
            float dx = p2.x - p1.x;
            float dy = p2.y - p1.y;
            float len = sqrtf(dx*dx + dy*dy);
            if (len == 0) {
                continue;
            }
            ++segmentsRendered;
            float perpx = -dy/len * thickness/2;
            float perpy = dx/len * thickness/2;
            
            // Draw a thick line as a quad
            SDL_Vertex vertices[4] = {
                { { p1.x + perpx, p1.y + perpy }, color, { 0.f, 0.f } }, // top-left
                { { p1.x - perpx, p1.y - perpy }, color, { 0.f, 0.f } }, // bottom-left
                { { p2.x - perpx, p2.y - perpy }, color, { 0.f, 0.f } }, // bottom-right
                { { p2.x + perpx, p2.y + perpy }, color, { 0.f, 0.f } }  // top-right
            };
            
            int squareIndices[6] = {
                0, 3, 2, // First triangle
                2, 1, 0  // Second triangle
            };

            SDL_RenderGeometry(renderer, nullptr, vertices, 4, squareIndices, 6);
        }
        return segmentsRendered;
    }
    
    void MapRenderer::drawLaneMarkers(const std::vector<SDL_Point>& nodes, int lanes, int laneWidthPixels) {
        if (nodes.size() < 2) {
             return;
        }
        
        SDL_SetRenderDrawColor(
            renderer, 
            Constants::LANE_MARKER_COLOR.r, 
            Constants::LANE_MARKER_COLOR.g, 
            Constants::LANE_MARKER_COLOR.b, 
            Constants::LANE_MARKER_COLOR.a
        );
        
        float totalWidth = lanes * Constants::LANE_WIDTH * Constants::PIXELS_PER_METER;
        float laneWidth = totalWidth / lanes;
        
        for (int lane = 1; lane < lanes; lane++) {
            float offset = -totalWidth/2 + lane * laneWidth;
            
            for (size_t i = 0; i < nodes.size() - 1; i++) {                            
                SDL_Point p1 = nodes[i];
                SDL_Point p2 = nodes[i+1];
                
                // Calculate perpendicular vector
                float dx = p2.x - p1.x;
                float dy = p2.y - p1.y;
                float len = sqrtf(dx*dx + dy*dy);
                if (len == 0) {
                    continue;
                }
                
                float perpx = -dy/len * offset;
                float perpy = dx/len * offset;
                
                // Draw dashed lane markers
                float segmentLength = 5.0f; // meters
                int segments = static_cast<int>(len / (segmentLength * Constants::PIXELS_PER_METER));
                
                for (int s = 0; s < segments; s += 2) {
                    float t1 = s / static_cast<float>(segments);
                    float t2 = (s+1) / static_cast<float>(segments);
                    
                    SDL_Point sp1 = {
                        static_cast<int>(p1.x + t1 * dx + perpx),
                        static_cast<int>(p1.y + t1 * dy + perpy)
                    };
                    SDL_Point sp2 = {
                        static_cast<int>(p1.x + t2 * dx + perpx),
                        static_cast<int>(p1.y + t2 * dy + perpy)
                    };
                    
                    SDL_RenderLine(renderer, sp1.x, sp1.y, sp2.x, sp2.y);
                }
            }
        }
    }


}

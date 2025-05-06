#include "stdafx.h"

#include "render/sdl/SDLRenderer.h"

#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_render.h>

#include "Application.h"
#include <core/dataLayer/WorldData.h>
#include <core/dataLayer/DataTypes.h>


namespace tjs::render {
    namespace vis {
        using namespace tjs::core;
        // Constants
        const int SCREEN_WIDTH = 1024;
        const int SCREEN_HEIGHT = 768;
        const float LANE_WIDTH = 2.5f; // meters in world space
        const float PIXELS_PER_METER = 2.0f;

        const double EARTH_RADIUS = 6378137.0; // in meters
        const double M_PI = 3.1418;
        const double DEG_TO_RAD = M_PI / 180.0;

        // Color definitions
        const SDL_FColor ROAD_COLOR = {0.392f, 0.392f, 0.392f, 1.0f};
        const SDL_FColor LANE_MARKER_COLOR = {0.392f, 0.392f, 0.392f, 1.0f};
        const SDL_FColor MOTORWAY_COLOR = {0.314f, 0.314f, 0.471f, 1.0f};
        const SDL_FColor PRIMARY_COLOR = {0.353f, 0.353f, 0.353f, 1.0f};
        const SDL_FColor RESIDENTIAL_COLOR = {0.471f, 0.471f, 0.471f, 1.0f};

        class MapRenderer {
            private:
                SDL_Renderer* renderer;
                Application& _application;
                
                // Map bounds for coordinate conversion
                float minLat = 90.0f, maxLat = -90.0f;
                float minLon = 180.0f, maxLon = -180.0f;
                
                 // Projection state
                Coordinates projectionCenter{0, 0};
                double metersPerPixel = 1.0;
                double screenCenterX = SCREEN_WIDTH / 2.0;
                double screenCenterY = SCREEN_HEIGHT / 2.0;
            public:
                MapRenderer(Application& application, SDL_Renderer* sdlRenderer)
                    : _application(application)
                    , renderer(sdlRenderer) {

                }
                
                ~MapRenderer() = default;
                
                void setView(const Coordinates& center, double zoomMetersPerPixel) {
                    projectionCenter = center;
                    metersPerPixel = zoomMetersPerPixel;
                    
                    // Adjust for Mercator projection stretching at high latitudes
                    double latRad = center.latitude * DEG_TO_RAD;
                    metersPerPixel *= std::cos(latRad);
                }
                
                SDL_Point convertToScreen(const Coordinates& coord) const {
                    // Convert geographic coordinates to meters using Mercator projection
                    double x = (coord.longitude - projectionCenter.longitude) * DEG_TO_RAD * EARTH_RADIUS;
                    double y = -std::log(std::tan((90.0 + coord.latitude) * DEG_TO_RAD / 2.0)) * EARTH_RADIUS;
                    double yCenter = -std::log(std::tan((90.0 + projectionCenter.latitude) * DEG_TO_RAD / 2.0)) * EARTH_RADIUS;
                    y -= yCenter;
                    
                    // Scale to screen coordinates
                    int screenX = static_cast<int>(screenCenterX + x / metersPerPixel);
                    int screenY = static_cast<int>(screenCenterY + y / metersPerPixel);
                    
                    return {screenX, screenY};
                }
                
                void autoZoom(const std::unordered_map<uint64_t, std::unique_ptr<Node>>& nodes) {
                    if (nodes.empty()) {
                        return;
                    }

                    calculateMapBounds(nodes);
                    
                    // Calculate bounds in meters
                    double minX = std::numeric_limits<double>::max();
                    double maxX = std::numeric_limits<double>::lowest();
                    double minY = std::numeric_limits<double>::max();
                    double maxY = std::numeric_limits<double>::lowest();
                    
                    Coordinates firstNode = nodes.begin()->second->coordinates;
                    projectionCenter = firstNode;
                    double yCenter = -std::log(std::tan((90.0 + firstNode.latitude) * DEG_TO_RAD / 2.0)) * EARTH_RADIUS;
                    
                    for (const auto& pair : nodes) {
                        const auto& node = pair.second;
                        double x = (node->coordinates.longitude - firstNode.longitude) * DEG_TO_RAD * EARTH_RADIUS;
                        double y = -std::log(std::tan((90.0 + node->coordinates.latitude) * DEG_TO_RAD / 2.0)) * EARTH_RADIUS - yCenter;
                        
                        minX = std::min(minX, x);
                        maxX = std::max(maxX, x);
                        minY = std::min(minY, y);
                        maxY = std::max(maxY, y);
                    }
                    
                    // Calculate required zoom level
                    double widthMeters = maxX - minX;
                    double heightMeters = maxY - minY;
                    
                    double zoomX = widthMeters / (SCREEN_WIDTH * 0.9);
                    double zoomY = heightMeters / (SCREEN_HEIGHT * 0.9);
                    
                    metersPerPixel = std::max(zoomX, zoomY);
                    screenCenterX = SCREEN_WIDTH / 2.0 - (minX + maxX) / (2.0 * metersPerPixel);
                    screenCenterY = SCREEN_HEIGHT / 2.0 - (minY + maxY) / (2.0 * metersPerPixel);
                    
                    // Adjust for latitude
                    double latRad = projectionCenter.latitude * DEG_TO_RAD;
                    metersPerPixel /= std::cos(latRad);
                }

                void calculateMapBounds(const std::unordered_map<uint64_t, std::unique_ptr<Node>>& nodes) {
                    for (const auto& pair : nodes) {
                        const auto& node = pair.second;
                        minLat = std::min(minLat, static_cast<float>(node->coordinates.latitude));
                        maxLat = std::max(maxLat, static_cast<float>(node->coordinates.latitude));
                        minLon = std::min(minLon, static_cast<float>(node->coordinates.longitude));
                        maxLon = std::max(maxLon, static_cast<float>(node->coordinates.longitude));
                    }
                }

                SDL_FColor getWayColor(WayTags tags) const {
                    SDL_FColor roadColor = ROAD_COLOR;
                    if (hasFlag(tags, WayTags::Motorway)) {
                        roadColor = MOTORWAY_COLOR;
                    }
                    else if (hasFlag(tags, WayTags::Primary)) {
                        roadColor = PRIMARY_COLOR;
                    }
                    else if (hasFlag(tags, WayTags::Residential)) {
                        roadColor = RESIDENTIAL_COLOR;
                    }
                    return roadColor;
                }
                
                int renderWay(const WayInfo& way, const std::unordered_map<uint64_t, std::unique_ptr<Node>>& nodes) {
                    if (way.nodeRefs.size() < 2) { 
                        return 0;
                    }
                    
                    std::vector<SDL_Point> screenPoints;
                    for (Node* node : way.nodes) {
                        screenPoints.push_back(convertToScreen(node->coordinates));
                    }
                    
                    // Draw the way
                    if (screenPoints.size() < 2)  {
                        return 0;
                    }
                    SDL_FColor color = getWayColor(way.tags);
                    int segmentsRendered = drawThickLine(screenPoints, way.lanes * LANE_WIDTH, color);
                    
                    if (way.lanes > 1) {
                        drawLaneMarkers(screenPoints, way.lanes, LANE_WIDTH);
                    }

                    return segmentsRendered;
                }
                
            private:
                int drawThickLine(const std::vector<SDL_Point>& nodes, float thickness, SDL_FColor color) {
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
                
                void drawLaneMarkers(const std::vector<SDL_Point>& nodes, int lanes, int laneWidthPixels) {
                    if (nodes.size() < 2) {
                         return;
                    }
                    
                    SDL_SetRenderDrawColor(renderer, 
                                         LANE_MARKER_COLOR.r, 
                                         LANE_MARKER_COLOR.g, 
                                         LANE_MARKER_COLOR.b, 
                                         LANE_MARKER_COLOR.a);
                    
                    float totalWidth = lanes * LANE_WIDTH * PIXELS_PER_METER;
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
                            int segments = static_cast<int>(len / (segmentLength * PIXELS_PER_METER));
                            
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
            };
    }

    SDLRenderer::SDLRenderer(Application& application)
        : _application(application)
    {}

    void SDLRenderer::initialize() {
            // Initialize SDL subsystems
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            // qWarning("SDL could not initialize! SDL Error: %s", SDL_GetError());
            return;
        }
        
        /* set up some random points */
        for (int i = 0; i < SDL_arraysize(_points); i++) {
            _points[i].x = (SDL_randf() * 440.0f) + 100.0f;
            _points[i].y = (SDL_randf() * 280.0f) + 100.0f;
        }

        // Create properties for window creation
        SDL_PropertiesID props = SDL_CreateProperties();
        //SDL_SetPointerProperty(props, SDL_PROP_WINDOW_CREATE_PARENT_POINTER, (void*)this->winId());
        
        // Create the SDL window using properties
        _sdlWindow = SDL_CreateWindowWithProperties(props);
        SDL_DestroyProperties(props);
        

        if (!_sdlWindow) {
            //qWarning("Window could not be created! SDL Error: %s", SDL_GetError());
            SDL_Quit();
            return;
        }
        
        // Set the window size to match the widget
        SDL_SetWindowSize(_sdlWindow, vis::SCREEN_WIDTH, vis::SCREEN_HEIGHT);
        
        // Create the renderer
        _sdlRenderer = SDL_CreateRenderer(_sdlWindow, nullptr);
        if (!_sdlRenderer) {
            // qWarning("Renderer could not be created! SDL Error: %s", SDL_GetError());
            SDL_DestroyWindow(_sdlWindow);
            SDL_Quit();
            return;
        }
        
        SDL_SetRenderDrawColor(_sdlRenderer, 0, 0, 0, 255);
        SDL_RenderClear(_sdlRenderer);
        SDL_RenderPresent(_sdlRenderer);
        
        _isInited = true;
    }

    void SDLRenderer::release() {
        if (!_isInited) {
            return;
        }

        if (_sdlRenderer) {
            SDL_DestroyRenderer(_sdlRenderer);
            _sdlRenderer = nullptr;
        }
        
        if (_sdlWindow) {
            SDL_DestroyWindow(_sdlWindow);
            _sdlWindow = nullptr;
        }
        
        SDL_Quit();
        _isInited = false;
    }

    void SDLRenderer::update() {
        // Handle SDL events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // Process SDL events as needed
        }
    }

    void SDLRenderer::draw() {
        if (!_sdlRenderer) {
            return;
        }
        
        auto& world = _application.worldData();
        auto& segments = world.segments();

        if (segments.empty()) {
            return;
        }

        auto& segment = segments.front();

        vis::MapRenderer renderer(_application, _sdlRenderer);
        renderer.autoZoom(segment->nodes);


         // Clear screen
         SDL_SetRenderDrawColor(_sdlRenderer, 240, 240, 240, 255);
         SDL_RenderClear(_sdlRenderer);
        

         // Render all ways
         int waysRendered = 0;
         int totalNodesRendered = 0;
         int realNodeSegments = 0;
         for (const auto& wayPair : segment->ways) {
            int nodesRendered = renderer.renderWay(*wayPair.second, segment->nodes);
            realNodeSegments += wayPair.second->nodes.size() / 2;
            if (nodesRendered > 0) {
                waysRendered++;
            }
            totalNodesRendered += nodesRendered;
         }

         
         
         // Present the renderer
         SDL_RenderPresent(_sdlRenderer);
    }
}


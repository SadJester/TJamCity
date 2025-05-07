#pragma once

#include <core/dataLayer/DataTypes.h>
#include <SDL3/SDL_render.h>

class SDL_Renderer;

namespace tjs {
    class Application;
    class IRenderer;

    namespace render::visualization {
        class MapRenderer {
        private:
            SDL_Renderer* renderer;
            Application& _application;
            
            // Map bounds for coordinate conversion
            float minLat = 90.0f, maxLat = -90.0f;
            float minLon = 180.0f, maxLon = -180.0f;
            
            // Projection state
            core::Coordinates projectionCenter{0, 0};
            double metersPerPixel = 1.0;
            double screenCenterX = 512.0f;
            double screenCenterY = 512.0f;
        public:
            MapRenderer(Application& application, SDL_Renderer* sdlRenderer);
            ~MapRenderer() = default;
            
            void autoZoom(const std::unordered_map<uint64_t, std::unique_ptr<core::Node>>& nodes);
            void setView(const core::Coordinates& center, double zoomMetersPerPixel);

            const core::Coordinates GetCurrentView() const {
                return projectionCenter;
            }
            
            SDL_Point convertToScreen(const core::Coordinates& coord) const;
            void calculateMapBounds(const std::unordered_map<uint64_t, std::unique_ptr<core::Node>>& nodes);

            SDL_FColor getWayColor(core::WayTags tags) const;
            
            int renderWay(const core::WayInfo& way, const std::unordered_map<uint64_t, std::unique_ptr<core::Node>>& nodes);
            void renderBoundingBox() const;

        private:
            int drawThickLine(const std::vector<SDL_Point>& nodes, float thickness, SDL_FColor color);
            void drawLaneMarkers(const std::vector<SDL_Point>& nodes, int lanes, int laneWidthPixels);
        };
    }
}
#pragma once

#include <core/dataLayer/DataTypes.h>
#include "render/RenderBase.h"

class SDL_Renderer;

namespace tjs {
    class Application;
    class IRenderer;

    namespace render::visualization {
        class MapRenderer {
        private:
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
            MapRenderer(Application& application);
            ~MapRenderer() = default;
            
            void autoZoom(const std::unordered_map<uint64_t, std::unique_ptr<core::Node>>& nodes);
            void setView(const core::Coordinates& center, double zoomMetersPerPixel);

            const core::Coordinates GetCurrentView() const {
                return projectionCenter;
            }
            
            Position convertToScreen(const core::Coordinates& coord) const;
            void calculateMapBounds(const std::unordered_map<uint64_t, std::unique_ptr<core::Node>>& nodes);

            FColor getWayColor(core::WayTags tags) const;
            
            int renderWay(const core::WayInfo& way, const std::unordered_map<uint64_t, std::unique_ptr<core::Node>>& nodes);
            void renderBoundingBox() const;

        private:
            int drawThickLine(const std::vector<Position>& nodes, float thickness, FColor color);
            void drawLaneMarkers(const std::vector<Position>& nodes, int lanes, int laneWidthPixels);
        };
    }
}
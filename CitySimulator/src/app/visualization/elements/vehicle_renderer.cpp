#include <stdafx.h>

#include <visualization/elements/vehicle_renderer.h>
#include <visualization/elements/MapElement.h>

#include <Application.h>
#include <visualization/Scene.h>
#include <visualization/SceneSystem.h>
#include <core/dataLayer/DataTypes.h>
#include <core/dataLayer/WorldData.h>


namespace tjs::visualization
{
    
    VehicleRenderer::VehicleRenderer(Application& application)
        : SceneNode("VehicleRenderer")
        , _application(application)
    {}

    VehicleRenderer::~VehicleRenderer() {
    }

    void VehicleRenderer::init() {
        auto scene = _application.sceneSystem().getScene("General");
        if (scene == nullptr) {
             return;
        }

        _mapElement = dynamic_cast<visualization::MapElement*>(scene->getNode("MapElement"));
    }

    void VehicleRenderer::update() {

    }

    void VehicleRenderer::render(IRenderer& renderer) {
        if (!_mapElement) {
            return;
        }

        auto& vehicles = _application.worldData().vehicles();
        for (auto& vehicle : vehicles) {
            render(renderer, vehicle);
        }
    }

    struct VehicleRenderSettings {
        FColor color;
        float width;
        float height;
    };
    
    struct VehicleSettings {
        std::array<VehicleRenderSettings, 6> renderSettings;
    };

    // Initialize the vehicle settings
    const VehicleSettings vehicleSettings = {
        {
            VehicleRenderSettings{FColor{0.0f, 0.0f, 1.0f, 1.0f}, 4.5f, 1.8f}, // SimpleCar - Blue
            VehicleRenderSettings{FColor{1.0f, 0.0f, 0.0f, 1.0f}, 6.5f, 2.5f}, // SmallTruck - Red
            VehicleRenderSettings{FColor{0.0f, 1.0f, 0.0f, 1.0f}, 10.0f, 3.5f}, // BigTruck - Green
            VehicleRenderSettings{FColor{1.0f, 1.0f, 0.0f, 1.0f}, 5.5f, 2.2f}, // Ambulance - Yellow
            VehicleRenderSettings{FColor{0.0f, 1.0f, 1.0f, 1.0f}, 5.0f, 2.0f}, // PoliceCar - Cyan
            VehicleRenderSettings{FColor{1.0f, 0.0f, 1.0f, 1.0f}, 8.0f, 3.0f} // FireTrack - Magenta
        }
    };

    void VehicleRenderer::render(IRenderer& renderer, const core::Vehicle& vehicle) {
        const float metersPerPixel =_mapElement->getZoomLevel();

        // Get the settings for the vehicle based on its type
        const VehicleRenderSettings& settings = vehicleSettings.renderSettings[static_cast<int>(vehicle.type)];

        // Set the color for the vehicle
        renderer.setDrawColor(settings.color);

        // Convert coordinates to screen coordinates
        auto [screenX, screenY] = _mapElement->convertToScreen(vehicle.coordinates);

        // Calculate width and height in pixels based on metersPerPixel
        const int widthInPixels = static_cast<int>(settings.width / metersPerPixel);
        const int heightInPixels = static_cast<int>(settings.height / metersPerPixel);

        // Define vertices for the rectangle
        Vertex vertices[4] = {
            {{screenX - widthInPixels / 2.0f, screenY - heightInPixels / 2.0f}, settings.color, {0.f, 0.f}}, // bottom-left
            {{screenX + widthInPixels / 2.0f, screenY - heightInPixels / 2.0f}, settings.color, {0.f, 0.f}}, // top-left
            {{screenX + widthInPixels / 2.0f, screenY + heightInPixels / 2.0f}, settings.color, {0.f, 0.f}}, // top-right
            {{screenX - widthInPixels / 2.0f, screenY + heightInPixels / 2.0f}, settings.color, {0.f, 0.f}}  // bottom-right
        };

        // Define indices for the rectangle (two triangles)
        int squareIndices[6] = {
            0, 1, 2, // First triangle
            2, 3, 0  // Second triangle
        };

        // Create geometry object
        Geometry geometry{
            std::span(vertices),
            std::span(squareIndices)
        };

        // Draw the vehicle as a rectangle
        renderer.drawGeometry(geometry);
    }

} // namespace tjs::visualization


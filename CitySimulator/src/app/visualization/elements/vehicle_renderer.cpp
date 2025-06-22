#include <stdafx.h>

#include <visualization/elements/vehicle_renderer.h>

#include <Application.h>
#include <visualization/Scene.h>
#include <visualization/scene_system.h>
#include <data/map_renderer_data.h>

#include <core/data_layer/data_types.h>
#include <core/store_models/idata_model.h>
#include <core/data_layer/world_data.h>

#include <visualization/elements/map_element.h>
#include <data/persistent_render_data.h>

namespace tjs::visualization {

	VehicleRenderer::VehicleRenderer(Application& application)
		: SceneNode("VehicleRenderer")
		, _application(application)
		, _mapRendererData(*application.stores().get_model<core::model::MapRendererData>()) {
	}

	VehicleRenderer::~VehicleRenderer() {
	}

	void VehicleRenderer::init() {
		_mapRendererData = *_application.stores().get_model<core::model::MapRendererData>();
	}

	void VehicleRenderer::update() {
	}

	void VehicleRenderer::render(IRenderer& renderer) {
		for (auto& vehicle : _application.worldData().vehicles()) {
			render(renderer, vehicle);
		}
	}

	struct VehicleRenderSettings {
		FColor color;
		float length;
		float width;
	};

	struct VehicleSettings {
		std::array<VehicleRenderSettings, 6> renderSettings;
	};

	// Initialize the vehicle settings
	const VehicleSettings vehicleSettings = {
		{
			VehicleRenderSettings { FColor { 0.0f, 0.0f, 1.0f, 1.0f }, 4.5f, 1.8f },  // SimpleCar - Blue
			VehicleRenderSettings { FColor { 1.0f, 0.0f, 0.0f, 1.0f }, 6.5f, 2.5f },  // SmallTruck - Red
			VehicleRenderSettings { FColor { 0.0f, 1.0f, 0.0f, 1.0f }, 10.0f, 2.5f }, // BigTruck - Green
			VehicleRenderSettings { FColor { 1.0f, 1.0f, 0.0f, 1.0f }, 5.5f, 2.2f },  // Ambulance - Yellow
			VehicleRenderSettings { FColor { 0.0f, 1.0f, 1.0f, 1.0f }, 5.0f, 2.0f },  // PoliceCar - Cyan
			VehicleRenderSettings { FColor { 1.0f, 0.0f, 1.0f, 1.0f }, 8.0f, 2.5f }   // FireTrack - Magenta
		}
	};

	void VehicleRenderer::render(IRenderer& renderer, const core::Vehicle& vehicle) {
		const float metersPerPixel = _mapRendererData.metersPerPixel;

		// Get the settings for the vehicle based on its type
		const VehicleRenderSettings& settings = vehicleSettings.renderSettings[static_cast<int>(vehicle.type)];

		// Set the color for the vehicle
		renderer.set_draw_color(settings.color);

		// Convert coordinates to screen coordinates
		auto screenPos = tjs::visualization::convert_to_screen(
			vehicle.coordinates,
			_mapRendererData.screen_center,
			_mapRendererData.metersPerPixel);
		int screenX = screenPos.x;
		int screenY = screenPos.y;

		if (!_application.renderer().is_point_visible(screenX, screenY)) {
			return;
		}

		// Calculate width and height in pixels based on metersPerPixel
		const float scaler = _application.settings().render.vehicleScaler;
		const float widthInPixels = scaler * settings.width / metersPerPixel;
		const float lengthInPixels = scaler * settings.length / metersPerPixel;

		// Define vertices for the rectangle. The vehicle length is aligned
		// with the X-axis so that a rotation angle of 0 corresponds to
		// a vehicle facing to the right.
		Vertex vertices[4] = {
			{ { screenX - lengthInPixels / 2.0f, screenY - widthInPixels / 2.0f }, settings.color, { 0.f, 0.f } }, // bottom-left
			{ { screenX + lengthInPixels / 2.0f, screenY - widthInPixels / 2.0f }, settings.color, { 0.f, 0.f } }, // bottom-right
			{ { screenX + lengthInPixels / 2.0f, screenY + widthInPixels / 2.0f }, settings.color, { 0.f, 0.f } }, // top-right
			{ { screenX - lengthInPixels / 2.0f, screenY + widthInPixels / 2.0f }, settings.color, { 0.f, 0.f } }  // top-left
		};

		const float angle = vehicle.rotationAngle;
		for (auto& v : vertices) {
			// Translate to origin, rotate, then translate back
			float dx = v.position.x - screenX;
			float dy = v.position.y - screenY;
			v.position.x = dx * cos(angle) - dy * sin(angle) + screenX;
			v.position.y = dx * sin(angle) + dy * cos(angle) + screenY;
		}

		// Define indices for the rectangle (two triangles)
		int squareIndices[6] = {
			0, 1, 2, // First triangle
			2, 3, 0  // Second triangle
		};

		// Create geometry object
		Geometry geometry {
			std::span(vertices),
			std::span(squareIndices)
		};

		// Draw the vehicle as a rectangle
		renderer.draw_geometry(geometry);
	}

} // namespace tjs::visualization

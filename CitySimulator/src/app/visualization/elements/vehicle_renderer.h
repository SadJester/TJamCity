#pragma once

#include <visualization/scene_node.h>
#include <logic/map/vehicle_targeting.h>

namespace tjs {
	class Application;
	class IRenderer;

	namespace core {
		struct Vehicle;

		namespace model {
			struct MapRendererData;
		} // namespace model

	} // namespace core

} // namespace tjs

namespace tjs::visualization {
	class MapElement;
	class VehicleTargeting;

	class VehicleRenderer : public SceneNode {
	public:
		VehicleRenderer(Application& application);
		~VehicleRenderer();

		virtual void init() override;
		virtual void update() override;
		virtual void render(IRenderer& renderer) override;

	private:
		void render(IRenderer& renderer, const core::Vehicle& vehicle);

	private:
		core::model::MapRendererData& _mapRendererData;
		Application& _application;
		VehicleTargeting _vehicleTargeting;
	};
} // namespace tjs::visualization

#pragma once

#include <visualization/scene_node.h>

#include <core/simulation/transport_management/vehicle_shared_state.h>

namespace tjs {
	class Application;
	class IRenderer;

	namespace core::model {
		struct MapRendererData;
	} // namespace core::model

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
		void render(IRenderer& renderer, const core::VehicleState1& vehicle);

	private:
		core::model::MapRendererData& _mapRendererData;
		Application& _application;
		core::VehicleShared::connection _connection;
	};
} // namespace tjs::visualization

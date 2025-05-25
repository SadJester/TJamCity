#pragma once

#include "visualization/scene_node.h"

namespace tjs {
	class Application;
	class IRenderer;

	namespace core {
		struct Vehicle;
	} // namespace core
} // namespace tjs

namespace tjs::visualization {
	class MapElement;

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
		MapElement* _mapElement;
		Application& _application;
	};
} // namespace tjs::visualization

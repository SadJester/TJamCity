#pragma once

#include "render/IRenderable.h"
#include "render/render_primitives.h"

namespace tjs::visualization {
	class SceneNode : public render::IRenderable {
	public:
		SceneNode(std::string_view name, FPoint position = { 0, 0 })
			: IRenderable(name)
			, _position(position) {
		}

		virtual void init() {}
		// from IRenderable
		virtual void update() override {}
		virtual void render(IRenderer& renderer) override {}

		// Getters and setters
		FPoint getPosition() const { return _position; }
		void setPosition(FPoint pos) { _position = pos; }

	protected:
		FPoint _position;
	};
} // namespace tjs::visualization

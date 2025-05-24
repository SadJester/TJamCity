#pragma once

#include <SDL3/SDL.h>

#include "render/RenderBase.h"

namespace tjs {
	class Application;

	namespace render {
		class SDLRenderer final : public IRenderer {
		public:
			explicit SDLRenderer(Application& application);
			virtual ~SDLRenderer();

			virtual void initialize() override;
			virtual void release() override;

			virtual void update() override;
			virtual void beginFrame() override;
			virtual void endFrame() override;

			virtual void setDrawColor(FColor color) override;
			virtual void drawLine(int x1, int y1, int x2, int y2) override;
			virtual void drawGeometry(const Geometry& geometry) override;
			virtual void drawCircle(int centerX, int centerY, int radius) override;

		private:
			Application& _application;
			SDL_Window* _sdlWindow = nullptr;
			SDL_Renderer* _sdlRenderer = nullptr;

			// Track if SDL is initialized
			bool _isInited = false;
		};
	} // namespace render
} // namespace tjs

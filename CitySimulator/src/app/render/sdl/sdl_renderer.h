#pragma once

#include <SDL3/SDL.h>

#include "render/render_base.h"

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
			virtual void begin_frame() override;
			virtual void end_frame() override;

			virtual void set_draw_color(FColor color) override;
			virtual void draw_line(int x1, int y1, int x2, int y2) override;
			virtual void draw_geometry(const Geometry& geometry) override;
			virtual void draw_circle(int center_x, int center_y, int radius, bool fill = false) override;
			virtual void draw_rect(const Rectangle& rect, bool fill = false) override;

		private:
			Application& _application;
			SDL_Window* _sdlWindow = nullptr;
			SDL_Renderer* _sdlRenderer = nullptr;

			// Track if SDL is initialized
			bool _isInited = false;
		};
	} // namespace render
} // namespace tjs

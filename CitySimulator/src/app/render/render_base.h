#pragma once

#include "render/render_primitives.h"
#include "render/render_events.h"
#include "render/render_events_manager.h"
#include <memory>

namespace tjs {
	class IRenderer {
	public:
		virtual ~IRenderer() {}

		virtual void initialize() = 0;
		virtual void release() = 0;

		virtual void update() = 0;
		virtual void begin_frame() = 0;
		virtual void end_frame() = 0;

		virtual void set_draw_color(FColor color) = 0;
		virtual void draw_line(int x1, int y1, int x2, int y2) = 0;
		virtual void draw_geometry(const Geometry& polygon, bool outline = false) = 0;
		virtual void draw_circle(int center_x, int center_y, int radius, bool fill = false) = 0;
		virtual void draw_rect(const Rectangle& rect, bool fill = false) = 0;

		// Event handling
		void register_event_listener(render::IRenderEventListener* listener) {
			_eventManager.register_listener(listener);
		}

		void unregister_event_listener(render::IRenderEventListener* listener) {
			_eventManager.unregister_listener(listener);
		}

		render::RendererEventsManager& event_manager() {
			return _eventManager;
		}

		int screen_width() const { return _screenWidth; }
		int screen_height() const { return _screenHeight; }

		bool is_point_visible(int x, int y) const {
			return x >= 0 && x < _screenWidth && y >= 0 && y < _screenHeight;
		}

		virtual void set_clear_color(FColor color) {
			_clearColor = color;
		}

	protected:
		void set_screen_dimensions(int width, int height) {
			_screenWidth = width;
			_screenHeight = height;
		}

	protected:
		int _screenWidth = 0;
		int _screenHeight = 0;
		FColor _clearColor;
		render::RendererEventsManager _eventManager;
	};
} // namespace tjs

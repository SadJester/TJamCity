#pragma once

#include "render_events.h"
#include <vector>

namespace tjs::render {
	class RendererEventsManager {
	public:
		void register_listener(IRenderEventListener* listener);
		void unregister_listener(IRenderEventListener* listener);

		void dispatch_mouse_event(const RendererMouseEvent& event);
		void dispatch_mouse_wheel_event(const RendererMouseWheelEvent& event);
		void dispatch_mouse_motion_event(const RendererMouseMotionEvent& event);
		void dispatch_key_event(const RendererKeyEvent& event);
		void dispatch_resize_event(const RenderResizeEvent& event);

	private:
		std::vector<IRenderEventListener*> _listeners;
	};
} // namespace tjs::render

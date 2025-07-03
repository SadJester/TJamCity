#include "render_events_manager.h"
#include <algorithm>

namespace tjs::render {
	void RendererEventsManager::register_listener(IRenderEventListener* listener) {
		if (listener && std::find(_listeners.begin(), _listeners.end(), listener) == _listeners.end()) {
			_listeners.push_back(listener);
		}
	}

	void RendererEventsManager::unregister_listener(IRenderEventListener* listener) {
		if (listener) {
			_listeners.erase(
				std::remove(_listeners.begin(), _listeners.end(), listener),
				_listeners.end());
		}
	}

	void RendererEventsManager::dispatch_mouse_event(const RendererMouseEvent& event) {
		for (auto* listener : _listeners) {
			listener->on_mouse_event(event);
		}
	}

	void RendererEventsManager::dispatch_mouse_wheel_event(const RendererMouseWheelEvent& event) {
		for (auto* listener : _listeners) {
			listener->on_mouse_wheel_event(event);
		}
	}

	void RendererEventsManager::dispatch_mouse_motion_event(const RendererMouseMotionEvent& event) {
		for (auto* listener : _listeners) {
			listener->on_mouse_motion_event(event);
		}
	}

	void RendererEventsManager::dispatch_key_event(const RendererKeyEvent& event) {
		for (auto* listener : _listeners) {
			listener->on_key_event(event);
		}
	}
} // namespace tjs::render

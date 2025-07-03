#pragma once

namespace tjs::render {
	// Base event class for all renderer events
	struct IRendererEvent {
		virtual ~IRendererEvent() = default;
	};

	// Mouse button event
	struct RendererMouseEvent : IRendererEvent {
		enum class ButtonType {
			Left,
			Right,
			Middle
		};

		enum class ButtonState {
			Pressed,
			Released
		};

		ButtonType button;
		ButtonState state;
		int x;
		int y;
		bool shift = false;
		bool ctrl = false;
		bool alt = false;
	};

	// Mouse wheel event
	struct RendererMouseWheelEvent : IRendererEvent {
		float deltaX;
		float deltaY;
		int x; // Mouse position when wheel event occurred
		int y;
	};

	struct RendererMouseMotionEvent : IRendererEvent {
		int x;
		int y;
		int xrel;
		int yrel;
	};

	// Keyboard event
	struct RendererKeyEvent : IRendererEvent {
		enum class KeyState {
			Pressed,
			Released
		};

		int keyCode;
		KeyState state;
		bool isRepeat;
		// Add modifiers if needed (Shift, Ctrl, Alt, etc.)
		bool shift;
		bool ctrl;
		bool alt;
	};

	struct RenderResizeEvent : IRendererEvent {
		int new_width;
		int new_height;

		RenderResizeEvent(int w, int h)
			: new_width(w)
			, new_height(h) {
		}
	};

	// Event listener interface
	class IRenderEventListener {
	public:
		virtual ~IRenderEventListener() = default;

		virtual void on_mouse_event(const RendererMouseEvent& event) {}
		virtual void on_mouse_wheel_event(const RendererMouseWheelEvent& event) {}
		virtual void on_mouse_motion_event(const RendererMouseMotionEvent& event) {}
		virtual void on_key_event(const RendererKeyEvent& event) {}
		virtual void on_resize_event(const RenderResizeEvent& event) {}
	};
} // namespace tjs::render

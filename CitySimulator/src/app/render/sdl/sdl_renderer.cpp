#include "stdafx.h"
#include "render/sdl/sdl_renderer.h"
#include "render/render_events.h"
#include "Application.h"

#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_render.h>

#include <core/data_layer/world_data.h>
#include <core/data_layer/data_types.h>

namespace tjs::render {
	const int SCREEN_WIDTH = 1024;
	const int SCREEN_HEIGHT = 768;

	SDLRenderer::SDLRenderer(Application& application)
		: _application(application)
		, _metrics(*application.stores().get_entry<core::model::RenderMetricsData>()) {
		_trianglesCount = 0;
	}

	SDLRenderer::~SDLRenderer() {
	}

	void SDLRenderer::initialize() {
		// Initialize SDL subsystems
		if (!SDL_Init(SDL_INIT_VIDEO)) {
			// qWarning("SDL could not initialize! SDL Error: %s", SDL_GetError());
			return;
		}

		const auto& win_settings = _application.settings().general.sdl_window;

		// Create properties for window creation
		SDL_PropertiesID props = SDL_CreateProperties();
		//SDL_SetPointerProperty(props, SDL_PROP_WINDOW_CREATE_PARENT_POINTER, (void*)this->winId());

		// Set size
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, win_settings.width);
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, win_settings.height);
		SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);

		// Create the SDL window using properties
		_sdlWindow = SDL_CreateWindowWithProperties(props);
		SDL_DestroyProperties(props);

		if (_sdlWindow) {
			SDL_SetWindowPosition(_sdlWindow, win_settings.x, win_settings.y);
		}

		if (!_sdlWindow) {
			//qWarning("Window could not be created! SDL Error: %s", SDL_GetError());
			SDL_Quit();
			return;
		}

		// Set the window size to match the widget
		this->set_screen_dimensions(win_settings.width, win_settings.height);

		// Create the renderer
		_sdlRenderer = SDL_CreateRenderer(_sdlWindow, nullptr);
		if (!_sdlRenderer) {
			// qWarning("Renderer could not be created! SDL Error: %s", SDL_GetError());
			SDL_DestroyWindow(_sdlWindow);
			SDL_Quit();
			return;
		}

		SDL_SetRenderDrawColor(_sdlRenderer, 0, 0, 0, 255);
		SDL_RenderClear(_sdlRenderer);
		SDL_RenderPresent(_sdlRenderer);
		_isInited = true;
	}

	void SDLRenderer::release() {
		if (!_isInited) {
			return;
		}

		if (_sdlRenderer) {
			SDL_DestroyRenderer(_sdlRenderer);
			_sdlRenderer = nullptr;
		}

		if (_sdlWindow) {
			auto& win = _application.settings().general.sdl_window;
			int pos_x = 0;
			int pos_y = 0;
			int width = 0;
			int height = 0;
			SDL_GetWindowPosition(_sdlWindow, &pos_x, &pos_y);
			SDL_GetWindowSize(_sdlWindow, &width, &height);
			win.x = pos_x;
			win.y = pos_y;
			win.width = width;
			win.height = height;

			SDL_DestroyWindow(_sdlWindow);
			_sdlWindow = nullptr;
		}

		SDL_Quit();
		_isInited = false;
	}

	void SDLRenderer::update() {
		// Handle SDL events
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
				case SDL_EVENT_MOUSE_BUTTON_UP: {
					// Convert event coordinates to render coordinates
					SDL_ConvertEventToRenderCoordinates(_sdlRenderer, &event);

					// Create and dispatch mouse event
					RendererMouseEvent mouseEvent;
					mouseEvent.x = event.button.x;
					mouseEvent.y = event.button.y;

					SDL_Keymod mods = event.key.mod;
					mouseEvent.shift = (mods & SDL_KMOD_SHIFT) != 0;
					mouseEvent.ctrl = (mods & SDL_KMOD_CTRL) != 0;
					mouseEvent.alt = (mods & SDL_KMOD_ALT) != 0;

					// Set button type
					switch (event.button.button) {
						case SDL_BUTTON_LEFT:
							mouseEvent.button = RendererMouseEvent::ButtonType::Left;
							break;
						case SDL_BUTTON_RIGHT:
							mouseEvent.button = RendererMouseEvent::ButtonType::Right;
							break;
						case SDL_BUTTON_MIDDLE:
							mouseEvent.button = RendererMouseEvent::ButtonType::Middle;
							break;
						default:
							break;
					}

					// Set button state
					mouseEvent.state = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) ? RendererMouseEvent::ButtonState::Pressed : RendererMouseEvent::ButtonState::Released;

					_eventManager.dispatch_mouse_event(mouseEvent);
					break;
				}
				case SDL_EVENT_MOUSE_MOTION: {
					SDL_ConvertEventToRenderCoordinates(_sdlRenderer, &event);

					RendererMouseMotionEvent motionEvent;
					motionEvent.x = event.motion.x;
					motionEvent.y = event.motion.y;
					motionEvent.xrel = event.motion.xrel;
					motionEvent.yrel = event.motion.yrel;

					_eventManager.dispatch_mouse_motion_event(motionEvent);
					break;
				}
				case SDL_EVENT_MOUSE_WHEEL: {
					// Convert event coordinates to render coordinates
					SDL_ConvertEventToRenderCoordinates(_sdlRenderer, &event);

					// Create and dispatch wheel event
					RendererMouseWheelEvent wheelEvent;
					wheelEvent.x = event.wheel.mouse_x;
					wheelEvent.y = event.wheel.mouse_y;
					wheelEvent.deltaX = event.wheel.x;
					wheelEvent.deltaY = event.wheel.y;

					_eventManager.dispatch_mouse_wheel_event(wheelEvent);
					break;
				}
				case SDL_EVENT_KEY_DOWN:
				case SDL_EVENT_KEY_UP: {
					// Create and dispatch key event
					RendererKeyEvent keyEvent;
					keyEvent.keyCode = event.key.key;
					keyEvent.state = (event.type == SDL_EVENT_KEY_DOWN) ? RendererKeyEvent::KeyState::Pressed : RendererKeyEvent::KeyState::Released;
					keyEvent.isRepeat = event.key.repeat != 0;
					keyEvent.shift = (event.key.mod & SDL_KMOD_SHIFT) != 0;
					keyEvent.ctrl = (event.key.mod & SDL_KMOD_CTRL) != 0;
					keyEvent.alt = (event.key.mod & SDL_KMOD_ALT) != 0;

					_eventManager.dispatch_key_event(keyEvent);
					break;
				}
				case SDL_EVENT_QUIT: {
					_application.setFinished();
					break;
				}
				case SDL_EVENT_WINDOW_MOVED: {
					auto& win = _application.settings().general.sdl_window;
					win.x = event.window.data1;
					win.y = event.window.data2;
					break;
				}
				case SDL_EVENT_WINDOW_RESIZED: {
					int new_width = event.window.data1;
					int new_height = event.window.data2;
					this->set_screen_dimensions(new_width, new_height);
					auto& win = _application.settings().general.sdl_window;
					win.width = new_width;
					win.height = new_height;
					_eventManager.dispatch_resize_event(RenderResizeEvent { new_width, new_height });
					break;
				}
			}
		}
	}

	void SDLRenderer::begin_frame() {
		if (!_sdlRenderer) {
			return;
		}

		_trianglesCount = 0;

		SDL_SetRenderDrawColorFloat(_sdlRenderer, _clearColor.a, _clearColor.r, _clearColor.g, _clearColor.b);
		SDL_RenderClear(_sdlRenderer);
	}

	void SDLRenderer::end_frame() {
		if (!_sdlRenderer) {
			return;
		}

		_metrics.triangles_last_frame = _trianglesCount;

		// Present the renderer
		SDL_RenderPresent(_sdlRenderer);
	}

	void SDLRenderer::set_draw_color(FColor color) {
		SDL_SetRenderDrawColorFloat(_sdlRenderer, color.r, color.g, color.b, color.a);
	}

	void SDLRenderer::draw_line(int x1, int y1, int x2, int y2) {
		SDL_RenderLine(_sdlRenderer, x1, y1, x2, y2);
	}

	void SDLRenderer::draw_geometry(const Geometry& geometry, bool outline) {
		// TODO[error-handling]: renderer should assert if not %3 count
		if (outline && geometry.indices.size() % 3 == 0) {
			// Set color from first vertex (or customize per use)
			FColor color = geometry.vertices[0].color;
			SDL_SetRenderDrawColor(
				_sdlRenderer,
				static_cast<Uint8>(color.r * 255),
				static_cast<Uint8>(color.g * 255),
				static_cast<Uint8>(color.b * 255),
				static_cast<Uint8>(color.a * 255));

			for (size_t i = 0; i < geometry.indices.size(); i += 3) {
				int i0 = geometry.indices[i + 0];
				int i1 = geometry.indices[i + 1];
				int i2 = geometry.indices[i + 2];

				const FPoint& p0 = geometry.vertices[i0].position;
				const FPoint& p1 = geometry.vertices[i1].position;
				const FPoint& p2 = geometry.vertices[i2].position;

				SDL_FPoint tri[4] = {
					{ p0.x, p0.y },
					{ p1.x, p1.y },
					{ p2.x, p2.y },
					{ p0.x, p0.y } // close loop
				};

				SDL_RenderLines(_sdlRenderer, tri, 4);
			}
		} else {
			SDL_RenderGeometry(
				_sdlRenderer,
				nullptr,
				reinterpret_cast<SDL_Vertex*>(geometry.vertices.data()),
				geometry.vertices.size(),
				geometry.indices.data(),
				geometry.indices.size());
		}
		_trianglesCount += geometry.indices.size() / 3;
	}

	void SDLRenderer::draw_circle(int centerX, int centerY, int radius, bool fill) {
		if (fill) {
			// Draw filled circle using scanlines
			int x = radius;
			int y = 0;
			int err = 0;

			while (x >= y) {
				// Draw horizontal lines between points on the circle
				SDL_RenderLine(_sdlRenderer, centerX - x, centerY + y, centerX + x, centerY + y);
				SDL_RenderLine(_sdlRenderer, centerX - x, centerY - y, centerX + x, centerY - y);
				SDL_RenderLine(_sdlRenderer, centerX - y, centerY + x, centerX + y, centerY + x);
				SDL_RenderLine(_sdlRenderer, centerX - y, centerY - x, centerX + y, centerY - x);

				if (err <= 0) {
					y += 1;
					err += 2 * y + 1;
				}
				if (err > 0) {
					x -= 1;
					err -= 2 * x + 1;
				}
			}
		} else {
			// Draw circle outline using points
			int x = radius;
			int y = 0;
			int err = 0;

			while (x >= y) {
				SDL_RenderPoint(_sdlRenderer, centerX + x, centerY + y);
				SDL_RenderPoint(_sdlRenderer, centerX + y, centerY + x);
				SDL_RenderPoint(_sdlRenderer, centerX - y, centerY + x);
				SDL_RenderPoint(_sdlRenderer, centerX - x, centerY + y);
				SDL_RenderPoint(_sdlRenderer, centerX - x, centerY - y);
				SDL_RenderPoint(_sdlRenderer, centerX - y, centerY - x);
				SDL_RenderPoint(_sdlRenderer, centerX + y, centerY - x);
				SDL_RenderPoint(_sdlRenderer, centerX + x, centerY - y);

				if (err <= 0) {
					y += 1;
					err += 2 * y + 1;
				}
				if (err > 0) {
					x -= 1;
					err -= 2 * x + 1;
				}
			}
		}
	}

	void SDLRenderer::draw_rect(const Rectangle& rect, bool fill) {
		SDL_FRect sdlRect = {
			static_cast<float>(rect.x),
			static_cast<float>(rect.y),
			static_cast<float>(rect.width),
			static_cast<float>(rect.height)
		};

		if (fill) {
			SDL_RenderFillRect(_sdlRenderer, &sdlRect);
		} else {
			SDL_RenderRect(_sdlRenderer, &sdlRect);
		}
	}

} // namespace tjs::render

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
		: _application(application) {
	}

	SDLRenderer::~SDLRenderer() {
	}

	void SDLRenderer::initialize() {
		// Initialize SDL subsystems
		if (!SDL_Init(SDL_INIT_VIDEO)) {
			// qWarning("SDL could not initialize! SDL Error: %s", SDL_GetError());
			return;
		}

		// Create properties for window creation
		SDL_PropertiesID props = SDL_CreateProperties();
		//SDL_SetPointerProperty(props, SDL_PROP_WINDOW_CREATE_PARENT_POINTER, (void*)this->winId());

		// Create the SDL window using properties
		_sdlWindow = SDL_CreateWindowWithProperties(props);
		SDL_DestroyProperties(props);

		if (!_sdlWindow) {
			//qWarning("Window could not be created! SDL Error: %s", SDL_GetError());
			SDL_Quit();
			return;
		}

		// Set the window size to match the widget
		SDL_SetWindowSize(_sdlWindow, SCREEN_WIDTH, SCREEN_HEIGHT);
		this->set_screen_dimensions(SCREEN_WIDTH, SCREEN_HEIGHT);

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
				case SDL_EVENT_MOUSE_BUTTON_UP:
				case SDL_EVENT_MOUSE_MOTION: {
					// Convert event coordinates to render coordinates
					SDL_ConvertEventToRenderCoordinates(_sdlRenderer, &event);

					// Create and dispatch mouse event
					RendererMouseEvent mouseEvent;
					mouseEvent.x = event.button.x;
					mouseEvent.y = event.button.y;

					if (event.type == SDL_EVENT_MOUSE_MOTION) {
						// Handle motion separately if needed
						break;
					}

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
			}
		}
	}

	void SDLRenderer::begin_frame() {
		if (!_sdlRenderer) {
			return;
		}

		SDL_SetRenderDrawColorFloat(_sdlRenderer, _clearColor.a, _clearColor.r, _clearColor.g, _clearColor.b);
		SDL_RenderClear(_sdlRenderer);
	}

	void SDLRenderer::end_frame() {
		if (!_sdlRenderer) {
			return;
		}

		// Present the renderer
		SDL_RenderPresent(_sdlRenderer);
	}

	void SDLRenderer::set_draw_color(FColor color) {
		SDL_SetRenderDrawColorFloat(_sdlRenderer, color.r, color.g, color.b, color.a);
	}

	void SDLRenderer::draw_line(int x1, int y1, int x2, int y2) {
		SDL_RenderLine(_sdlRenderer, x1, y1, x2, y2);
	}

	void SDLRenderer::draw_geometry(const Geometry& geometry) {
		SDL_RenderGeometry(
			_sdlRenderer,
			nullptr,
			reinterpret_cast<SDL_Vertex*>(geometry.vertices.data()),
			4,
			geometry.indices.data(),
			geometry.indices.size());
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

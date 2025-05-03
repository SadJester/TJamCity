#include "render/sdl/SDLRenderer.h"

#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_render.h>

namespace tjs {
    namespace render {
        void SDLRenderer::initialize() {
                // Initialize SDL subsystems
            if (!SDL_Init(SDL_INIT_VIDEO)) {
                // qWarning("SDL could not initialize! SDL Error: %s", SDL_GetError());
                return;
            }
            
            /* set up some random points */
            for (int i = 0; i < SDL_arraysize(_points); i++) {
                _points[i].x = (SDL_randf() * 440.0f) + 100.0f;
                _points[i].y = (SDL_randf() * 280.0f) + 100.0f;
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
            SDL_SetWindowSize(_sdlWindow, 800, 800);
            
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
                // Process SDL events as needed
            }
        }

        void SDLRenderer::draw() {
            if (!_sdlRenderer) {
                return;
            }
            
            SDL_FRect rect;

            /* as you can see from this, rendering draws over whatever was drawn before it. */
            SDL_SetRenderDrawColor(_sdlRenderer, 33, 33, 33, SDL_ALPHA_OPAQUE);  /* dark gray, full alpha */
            SDL_RenderClear(_sdlRenderer);  /* start with a blank canvas. */

            /* draw a filled rectangle in the middle of the canvas. */
            SDL_SetRenderDrawColor(_sdlRenderer, 0, 0, 255, SDL_ALPHA_OPAQUE);  /* blue, full alpha */
            rect.x = rect.y = 100;
            rect.w = 440;
            rect.h = 280;
            SDL_RenderFillRect(_sdlRenderer, &rect);

            /* draw some points across the canvas. */
            SDL_SetRenderDrawColor(_sdlRenderer, 255, 0, 0, SDL_ALPHA_OPAQUE);  /* red, full alpha */
            SDL_RenderPoints(_sdlRenderer, _points, SDL_arraysize(_points));

            /* draw a unfilled rectangle in-set a little bit. */
            SDL_SetRenderDrawColor(_sdlRenderer, 0, 255, 0, SDL_ALPHA_OPAQUE);  /* green, full alpha */
            rect.x += 30;
            rect.y += 30;
            rect.w -= 60;
            rect.h -= 60;
            SDL_RenderRect(_sdlRenderer, &rect);

            /* draw two lines in an X across the whole canvas. */
            SDL_SetRenderDrawColor(_sdlRenderer, 255, 255, 0, SDL_ALPHA_OPAQUE);  /* yellow, full alpha */
            SDL_RenderLine(_sdlRenderer, 0, 0, 640, 480);
            SDL_RenderLine(_sdlRenderer, 0, 480, 640, 0);

            SDL_RenderPresent(_sdlRenderer);  /* put it all on the screen! */
        }
    }
}


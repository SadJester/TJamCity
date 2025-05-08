#include "stdafx.h"

#include "render/sdl/SDLRenderer.h"
#include "render/sdl/MapRenderer.h"
#include "render/sdl/RenderConstants.h"

#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_render.h>

#include "Application.h"
#include <core/dataLayer/WorldData.h>
#include <core/dataLayer/DataTypes.h>


namespace tjs::render {
    const int SCREEN_WIDTH = 1024;
    const int SCREEN_HEIGHT = 768;

    SDLRenderer::SDLRenderer(Application& application)
        : _application(application)
    {        
    }

    SDLRenderer::~SDLRenderer()
    {        
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
        this->setScreenDimensions(SCREEN_WIDTH, SCREEN_HEIGHT);
        
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
        
        _mapRenderer = std::make_unique<visualization::MapRenderer>(_application);

        auto& world = _application.worldData();
        auto& segments = world.segments();

        if (!segments.empty()) {
            _mapRenderer->autoZoom(segments.front()->nodes);
        }
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
        
        auto& world = _application.worldData();
        auto& segments = world.segments();

        if (segments.empty()) {
            return;
        }

        auto& segment = segments.front();

         // Clear screen
         SDL_SetRenderDrawColor(_sdlRenderer, 240, 240, 240, 255);
         SDL_RenderClear(_sdlRenderer);

         _mapRenderer->renderBoundingBox();
         // Render all ways
         int waysRendered = 0;
         int totalNodesRendered = 0;
         int realNodeSegments = 0;
         for (const auto& wayPair : segment->ways) {
            int nodesRendered = _mapRenderer->renderWay(*wayPair.second, segment->nodes);
            realNodeSegments += wayPair.second->nodes.size() / 2;
            if (nodesRendered > 0) {
                waysRendered++;
            }
            totalNodesRendered += nodesRendered;
         }
         // Present the renderer
         SDL_RenderPresent(_sdlRenderer);
    }


    void SDLRenderer::setDrawColor(FColor color) {
        SDL_SetRenderDrawColorFloat(_sdlRenderer, color.r, color.g, color.b, color.a);
    }

    void SDLRenderer::drawLine(int x1, int y1, int x2, int y2) {
        SDL_RenderLine(_sdlRenderer, x1, y1, x2, y2);
    }

    void SDLRenderer::drawGeometry(const Geometry& geometry) {
        SDL_RenderGeometry(
            _sdlRenderer,
            nullptr,
            reinterpret_cast<SDL_Vertex*>(geometry.vertices.data()),
            4,
            geometry.indices.data(),
            geometry.indices.size()
        );
    }

    void SDLRenderer::drawCircle(int centerX, int centerY, int radius) {
        // Midpoint circle algorithm
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
                err += 2*y + 1;
            }
            if (err > 0) {
                x -= 1;
                err -= 2*x + 1;
            }
        }
    }

}

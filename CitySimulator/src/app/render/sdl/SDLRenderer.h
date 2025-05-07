#pragma once

#include <SDL3/SDL.h>

#include "render/RenderBase.h"


namespace tjs {
    class Application;

    namespace render {
        namespace visualization {
            class MapRenderer;
        }

        class SDLRenderer final : public IRenderer {
            public:
                explicit SDLRenderer(Application& application);
                virtual ~SDLRenderer();

                virtual void initialize() override;
                virtual void release() override;
    
                virtual void update() override;
                virtual void draw() override;

            private:
                Application& _application;
                SDL_Window* _sdlWindow = nullptr;
                SDL_Renderer* _sdlRenderer = nullptr;
                
                // Track if SDL is initialized
                bool _isInited = false;

                std::unique_ptr<visualization::MapRenderer> _mapRenderer;
        };
    }
}
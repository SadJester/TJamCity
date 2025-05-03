#pragma once

#include <SDL3/SDL.h>

#include "render/RenderBase.h"


namespace tjs {
    namespace render {
        class SDLRenderer final : public IRenderer {
            public:
                virtual ~SDLRenderer(){}

                virtual void initialize() override;
                virtual void release() override;
    
                virtual void update() override;
                virtual void draw() override;

            private:
                SDL_Window* _sdlWindow = nullptr;
                SDL_Renderer* _sdlRenderer = nullptr;
                
                // Track if SDL is initialized
                bool _isInited = false;

                SDL_FPoint _points[500];
        };
    }
}
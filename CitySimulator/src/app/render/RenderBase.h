#pragma once

namespace tjs {
    class IRenderer {
        public:
            virtual ~IRenderer(){}

            virtual void initialize() = 0;
            virtual void release() = 0;

            virtual void update() = 0;
            virtual void draw() = 0;
    };
}
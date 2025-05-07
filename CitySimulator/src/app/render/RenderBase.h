#pragma once

namespace tjs {
    class IRenderer {
        public:
            virtual ~IRenderer(){}

            virtual void initialize() = 0;
            virtual void release() = 0;

            virtual void update() = 0;
            virtual void draw() = 0;

            int screenWidth() const { return _screenWidth; }
            int screenHeight() const { return _screenHeight; }

        protected:
            void setScreenDimensions(int width, int height) {
                _screenWidth = width;
                _screenHeight = height;
            }

        protected:
            int _screenWidth = 0;
            int _screenHeight = 0;
    };
}
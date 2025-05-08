#pragma once

namespace tjs::render {
    class IRenderable {
    public:
        virtual ~IRenderable() = default;

        virtual void render() = 0;
        virtual void update() = 0;
    };
}

#pragma once

#include <core/data_layer/data_types.h>

#include "render/render_primitives.h"
#include "render/IRenderable.h"

#include "visualization/scene_node.h"

namespace tjs {
    class Application;
    class IRenderer;
}


namespace tjs::visualization
{
    class MapElement;
    class PathRenderer : public SceneNode
    {
    public:
        PathRenderer(Application& application);
        ~PathRenderer() = default;

        virtual void init() override;
        virtual void update() override;
        virtual void render(IRenderer& renderer) override;

    private:
        Application& _application;
        MapElement* _mapElement;

    };
} // namespace tjs::visualization

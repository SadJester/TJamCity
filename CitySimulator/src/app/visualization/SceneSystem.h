#pragma once
#include "render/IRenderable.h"
#include "render/RenderBase.h"


namespace tjs {
    class IRenderer;
    class Application;
}

namespace tjs::visualization {
    class Scene;

    class SceneSystem {
    public:
        using ScenePtr = std::unique_ptr<Scene>;
        using Scenes = std::vector<ScenePtr>;

    public:
        SceneSystem(Application& application);
        ~SceneSystem();

        Application& application() { 
            return _application;
        }
        
        void initialize();
        void update();
        void render(IRenderer& renderer);

        void sortScenes();


        const Scenes& getScenes() const {
            return _scenes;
        }
        Scene* createScene(std::string name, int priority = 0);
        bool removeScene(std::string_view name);
        Scene* getScene(std::string_view name);
    private:
        Application& _application;

        Scenes _scenes;
    };
} // namespace tjs

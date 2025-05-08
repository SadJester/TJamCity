#pragma once


namespace tjs {
    class IRenderer;
}

namespace tjs::visualization {
    class SceneSystem;
    class SceneNode;

    class Scene {
    public:
        Scene(SceneSystem& sceneSystem, std::string_view name, int priority = 0);
        ~Scene();

        void initialize();
        void update();
        void render(IRenderer& renderer);

        // Container for scene elements
        void addNode(std::unique_ptr<SceneNode> node);
        void removeNode(std::string_view name);

        std::string_view getName() const { return _name; }
        SceneSystem& sceneSystem() { return _sceneSystem; }
        void setPriority(int priority);
        int getPrioirity() const {
             return _priority;
        }
    private:
        SceneSystem& _sceneSystem;
        std::string _name;
        std::vector<std::unique_ptr<SceneNode>> _nodes;
        int _priority;
    };
}
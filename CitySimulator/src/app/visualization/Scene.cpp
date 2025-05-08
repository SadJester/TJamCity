#include "stdafx.h"

#include "visualization/Scene.h"
#include "visualization/SceneSystem.h"
#include "visualization/SceneNode.h"


namespace tjs::visualization {
    Scene::Scene(SceneSystem& sceneSystem, std::string_view name, int priority)
        : _sceneSystem(sceneSystem)
        , _name(name)
        , _priority(priority) {
    }

    Scene::~Scene() {
    }

    void Scene::setPriority(int priority) {
        _priority = priority;
        _sceneSystem.sortScenes();
    }

    void Scene::addNode(std::unique_ptr<SceneNode> node) {
        _nodes.push_back(std::move(node));
    }

    void Scene::removeNode(std::string_view name) {
        auto it = std::remove_if(_nodes.begin(), _nodes.end(),
            [name](const auto& node) { return node->name() == name; });
        _nodes.erase(it, _nodes.end());
    }

    SceneNode* Scene::getNode(std::string_view name) {
        for(auto& node : _nodes) {
            if(node->name() == name) {
                return node.get();
            }
        }
        return nullptr;
    }

    void Scene::initialize() {
        for(auto& node : _nodes) {
            node->init();
        }
    }

    void Scene::update() {
        for(auto& node : _nodes) {
            node->update();
        }
    }

    void Scene::render(IRenderer& renderer) {
        for(auto& node : _nodes) {
            node->render(renderer);
        }
    }
}

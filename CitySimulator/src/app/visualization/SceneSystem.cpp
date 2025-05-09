#include "stdafx.h"

#include "visualization/SceneSystem.h"
#include "visualization/Scene.h"


namespace tjs::visualization {
    SceneSystem::SceneSystem(Application& application)
        : _application(application) {
    }

    SceneSystem::~SceneSystem() {
    }

    Scene* SceneSystem::createScene(std::string name, int priority) {
        auto scene = getScene(name);
        if (scene != nullptr) {
            return {};
        }

        _scenes.emplace_back(std::make_unique<Scene>(*this, name, priority));
        sortScenes();
        return _scenes.back().get();
    }

    bool SceneSystem::removeScene(std::string_view name) {
        std::erase_if(_scenes, [name](const auto& scene) {
            return scene->getName() == name;
        });
        return false;
    }

    Scene* SceneSystem::getScene(std::string_view name) {
        auto it = std::ranges::find_if(_scenes, [name](const auto& scene) { 
            return scene->getName() == name;
        });
        if (it != _scenes.end()) {
            return it->get();
        }
        return nullptr;
    }

    void SceneSystem::sortScenes() {
        std::ranges::sort(_scenes, [](const auto& a, const auto& b) {
            return a->getPrioirity() > b->getPrioirity();
        });
    }

    void SceneSystem::initialize() {
    }

    void SceneSystem::update() {
        for(auto& scene : _scenes) {
            scene->update();
        }
    }

    void SceneSystem::render(IRenderer& renderer) {
        for(auto& scene : _scenes) {
            scene->render(renderer);
        }
    }
}
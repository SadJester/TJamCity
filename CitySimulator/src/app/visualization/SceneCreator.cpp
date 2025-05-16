#include "stdafx.h"

#include "visualization/SceneCreator.h"

#include "visualization/SceneSystem.h"
#include "visualization/Scene.h"
#include "visualization/elements/MapElement.h"
#include <visualization/elements/vehicle_renderer.h>

#include "Application.h"


namespace tjs::visualization {
    void prepareScene(tjs::Application& app) {
        auto& sceneSystem = app.sceneSystem();

        auto scene = sceneSystem.createScene("General", 0);
        if (scene == nullptr) {
            return;
        }

        scene->addNode(std::make_unique<MapElement>(app));
        scene->addNode(std::make_unique<VehicleRenderer>(app));
        
        scene->initialize();
    }
}
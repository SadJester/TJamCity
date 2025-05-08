#include "stdafx.h"

#include "Application.h"
#include "uiSystem/UISystem.h"
#include "render/sdl/SDLRenderer.h"
#include "visualization/SceneSystem.h"
#include "visualization/SceneCreator.h"

#include <core/dataLayer/WorldData.h>
#include <core/dataLayer/WorldCreator.h>


int main(int argc, char* argv[]) {
    tjs::Application application(
        argc,
        argv,
        tjs::ApplicationConfig { 60 });

    application.setup(
        std::make_unique<tjs::render::SDLRenderer>(application),
        std::make_unique<tjs::UISystem>(application),
        std::make_unique<tjs::visualization::SceneSystem>(application),
        std::make_unique<tjs::core::WorldData>()
    );

    const char* fileName = "F:/PetProjects/near_me_map.osmx";
    if (!tjs::core::WorldCreator::loadOSMData(application.worldData(), fileName)) {
        return 1;
    }

    application.initialize();

    // TODO: Will move to some QT functional
    application.renderer().setClearColor({0.95f, 0.95f, 0.95f, 1.0f});
    tjs::visualization::prepareScene(application);

    application.run();

    return 0;
}
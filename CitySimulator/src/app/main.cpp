#include "stdafx.h"

#include "Application.h"
#include "uiSystem/UISystem.h"
#include "render/sdl/SDLRenderer.h"

#include <core/dataLayer/WorldData.h>
#include <core/dataLayer/WorldCreator.h>


int main(int argc, char* argv[]) {
    tjs::Application application(
        argc,
        argv,
        tjs::ApplicationConfig { 60 });

    application.setup(
        std::make_unique<tjs::render::SDLRenderer>(),
        std::make_unique<tjs::UISystem>(application),
        std::make_unique<tjs::core::WorldData>()
    );

    const char* fileName = "F:/PetProjects/near_me_map.osmx";
    tjs::core::WorldCreator::loadOSMData(application.worldData(), fileName);

    application.initialize();
    application.run();

    return 0;
}
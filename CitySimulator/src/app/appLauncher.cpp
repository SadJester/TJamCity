#include "stdafx.h"

#include "appLauncher.h"

#include <Application.h>
#include <uiSystem/UISystem.h>
#include <render/sdl/SDLRenderer.h>
#include <render/RenderConstants.h>
#include <visualization/SceneSystem.h>
#include <visualization/SceneCreator.h>

#include <core/dataLayer/WorldData.h>
#include <core/dataLayer/WorldCreator.h>


namespace tjs {
    int launch(int argc, char* argv[]) {
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

        application.initialize();

        // TODO: Will move to user settings in some time
        application.renderer().setClearColor(tjs::render::RenderConstants::BASE_CLEAR_COLOR);
        tjs::visualization::prepareScene(application);

        application.run();

        return 0;
    }
}

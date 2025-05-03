#include <memory>

#include "Application.h"
#include "uiSystem/UISystem.h"
#include "render/sdl/SDLRenderer.h"


int main(int argc, char* argv[]) {
    tjs::Application application(argc, argv);

    application.setup(
        std::make_unique<tjs::render::SDLRenderer>(),
        std::make_unique<tjs::UISystem>(application)
    );

    application.initialize();
    application.run();

    return 0;
}
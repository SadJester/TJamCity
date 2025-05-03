#include <memory>
#include "Application.h"

#include "render/RenderBase.h"

#include "uiSystem/UISystem.h"
#include "render/RenderBase.h"


namespace tjs {

    Application::Application(int& argc, char** argv)
        : _commandLine(argc, argv)
    {}

    void Application::setup(
            std::unique_ptr<IRenderer>&& renderer,
            std::unique_ptr<UISystem>&& uiSystem
    ) {
        _renderer = std::move(renderer);
        _uiSystem = std::move(uiSystem);
    }

    void Application::initialize() {
        _uiSystem->initialize();
        _renderer->initialize();
    }

    void Application::run() {
        while (!isFinished()) {
            _uiSystem->update();
            _renderer->update();
            _renderer->draw();
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

}

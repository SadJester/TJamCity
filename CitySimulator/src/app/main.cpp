#include <QApplication>
#include "MainWindow.h"

#include "uiSystem/UISystem.h"
#include "SDLWidget.h"
#include "Application.h"


int main(int argc, char* argv[]) {
    tjs::Application application;    
    tjs::UISystem uiSystem(application);
    uiSystem.initialize(argc, argv);

    tjs::Render::sdl::SDLRenderer renderer;
    renderer.Initialize({});
    
    while (!application.isFinished()) {
        uiSystem.update();
        renderer.Update();
        renderer.Draw();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}
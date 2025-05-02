#pragma once

#include <QWidget>
#include <QTimer>
#include <SDL3/SDL.h>


namespace tjs {
    namespace Render {

        struct ICreationContext {
            virtual ~ICreationContext(){}
        };

        class IRenderer {
            public:
                virtual ~IRenderer(){}

                virtual void Initialize(ICreationContext&& context) = 0;
                virtual void Release() = 0;
    
                virtual void Update() = 0;
                virtual void Draw() = 0;
        };


        namespace sdl {
            class SDLRenderer final : public IRenderer {
                public:
                    virtual ~SDLRenderer(){}

                    virtual void Initialize(ICreationContext&& context) override;
                    virtual void Release() override;
        
                    virtual void Update() override;
                    virtual void Draw() override;

                private:
                    SDL_Window* _sdlWindow = nullptr;
                    SDL_Renderer* _sdlRenderer = nullptr;
                    
                    // Track if SDL is initialized
                    bool _isInited = false;

                    SDL_FPoint _points[500];
            };
        }
    }
}





class SDLWidget : public QWidget {
    // TODO: Turn on Q_Object
    // * https://stackoverflow.com/questions/36340181/how-do-you-get-cmake-to-add-compiler-definitions-only-for-automoc-files
    // * https://stackoverflow.com/questions/23595961/qt-a-missing-vtable-usually-means-the-first-non-inline-virtual-member-function
    //Q_OBJECT

public:
    explicit SDLWidget(QWidget *parent = nullptr);
    ~SDLWidget() override;

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    
    // These aren't strictly necessary but provide useful hooks
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override;

private slots:
    void updateSDL();

private:
    void initializeSDL();
    void cleanupSDL();
    void renderSDL();

    SDL_Window *sdlWindow = nullptr;
    SDL_Renderer *sdlRenderer = nullptr;
    QTimer updateTimer;
    
    // Track if SDL is initialized
    bool sdlInitialized = false;
    
    // Position and angle for a simple animated rendering
    double angle = 0.0;
};
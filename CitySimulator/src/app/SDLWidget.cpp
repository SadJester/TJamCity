#include "SDLWidget.h"
#include <QResizeEvent>
#include <QApplication>
#include <QPainter>

#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_render.h>


SDLWidget::SDLWidget(QWidget *parent) : QWidget(parent) {
    // Set attributes to enable drawing directly on the widget
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_PaintOnScreen);
    
    // Setup update timer - adjust the interval as needed
    updateTimer.setInterval(16); // ~60 FPS
    connect(&updateTimer, &QTimer::timeout, this, &SDLWidget::updateSDL);
    
    // Set a size policy for the widget
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(320, 240);
    
    // Set focus policy to receive keyboard events
    setFocusPolicy(Qt::StrongFocus);
}

SDLWidget::~SDLWidget() {
    cleanupSDL();
}

void SDLWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    
    if (!sdlInitialized) {
        initializeSDL();
    }
    
    updateTimer.start();
}

void SDLWidget::hideEvent(QHideEvent *event) {
    updateTimer.stop();
    QWidget::hideEvent(event);
}

void SDLWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    
    // If SDL is initialized, update the renderer viewport
    if (sdlRenderer) {
        SDL_SetRenderViewport(sdlRenderer, nullptr);
    }
}

void SDLWidget::paintEvent(QPaintEvent *event) {
    // Let SDL handle the rendering
    Q_UNUSED(event);
}

bool SDLWidget::event(QEvent *event) {
    // Add special event handling here if needed
    return QWidget::event(event);
}

void SDLWidget::updateSDL() {
    // Handle SDL events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Process SDL events as needed
    }
    
    // Render the SDL content
    renderSDL();
}

static SDL_FPoint points[500];

void SDLWidget::initializeSDL() {
    // Initialize SDL subsystems
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        qWarning("SDL could not initialize! SDL Error: %s", SDL_GetError());
        return;
    }
    
    /* set up some random points */
    for (int i = 0; i < SDL_arraysize(points); i++) {
        points[i].x = (SDL_randf() * 440.0f) + 100.0f;
        points[i].y = (SDL_randf() * 280.0f) + 100.0f;
    }

    // Create properties for window creation
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetPointerProperty(props, SDL_PROP_WINDOW_CREATE_PARENT_POINTER, (void*)this->winId());
    
    // Create the SDL window using properties
    sdlWindow = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);
    

    if (!sdlWindow) {
        qWarning("Window could not be created! SDL Error: %s", SDL_GetError());
        SDL_Quit();
        return;
    }
    
    // Set the window size to match the widget
    SDL_SetWindowSize(sdlWindow, width(), height());
    
    // Create the renderer
    sdlRenderer = SDL_CreateRenderer(sdlWindow, nullptr);
    if (!sdlRenderer) {
        qWarning("Renderer could not be created! SDL Error: %s", SDL_GetError());
        SDL_DestroyWindow(sdlWindow);
        SDL_Quit();
        return;
    }
    
    SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
    SDL_RenderClear(sdlRenderer);
    SDL_RenderPresent(sdlRenderer);
    
    sdlInitialized = true;
}

void SDLWidget::cleanupSDL() {
    if (sdlInitialized) {
        if (sdlRenderer) {
            SDL_DestroyRenderer(sdlRenderer);
            sdlRenderer = nullptr;
        }
        
        if (sdlWindow) {
            SDL_DestroyWindow(sdlWindow);
            sdlWindow = nullptr;
        }
        
        SDL_Quit();
        sdlInitialized = false;
    }
}

void SDLWidget::renderSDL() {
    if (!sdlRenderer) {
        return;
    }
    
   SDL_FRect rect;

    /* as you can see from this, rendering draws over whatever was drawn before it. */
    SDL_SetRenderDrawColor(sdlRenderer, 33, 33, 33, SDL_ALPHA_OPAQUE);  /* dark gray, full alpha */
    SDL_RenderClear(sdlRenderer);  /* start with a blank canvas. */

    /* draw a filled rectangle in the middle of the canvas. */
    SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 255, SDL_ALPHA_OPAQUE);  /* blue, full alpha */
    rect.x = rect.y = 100;
    rect.w = 440;
    rect.h = 280;
    SDL_RenderFillRect(sdlRenderer, &rect);

    /* draw some points across the canvas. */
    SDL_SetRenderDrawColor(sdlRenderer, 255, 0, 0, SDL_ALPHA_OPAQUE);  /* red, full alpha */
    SDL_RenderPoints(sdlRenderer, points, SDL_arraysize(points));

    /* draw a unfilled rectangle in-set a little bit. */
    SDL_SetRenderDrawColor(sdlRenderer, 0, 255, 0, SDL_ALPHA_OPAQUE);  /* green, full alpha */
    rect.x += 30;
    rect.y += 30;
    rect.w -= 60;
    rect.h -= 60;
    SDL_RenderRect(sdlRenderer, &rect);

    /* draw two lines in an X across the whole canvas. */
    SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 0, SDL_ALPHA_OPAQUE);  /* yellow, full alpha */
    SDL_RenderLine(sdlRenderer, 0, 0, 640, 480);
    SDL_RenderLine(sdlRenderer, 0, 480, 640, 0);

    SDL_RenderPresent(sdlRenderer);  /* put it all on the screen! */
    
    // Update the angle for next frame
    angle += 1.0;
    if (angle >= 360.0) {
        angle -= 360.0;
    }
}
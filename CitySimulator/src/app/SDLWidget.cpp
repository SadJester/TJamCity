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

void SDLWidget::initializeSDL() {
    // Initialize SDL subsystems
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        qWarning("SDL could not initialize! SDL Error: %s", SDL_GetError());
        return;
    }
    
    // Create properties for window creation
    SDL_PropertiesID props = SDL_CreateProperties();
    
    // Set parent window handle
    SDL_SetPointerProperty(props, SDL_PROP_WINDOW_CREATE_PARENT_POINTER, (void*)this->winId());
    
    // Set window flags
    //SDL_SetNumberProperty(props, SDL_WINDOW_OPENGL, 0);  // Disable OpenGL for this example
    //SDL_SetNumberProperty(props, SDL_WINDOW_VULKAN, 0);  // Disable Vulkan for this example
    //SDL_SetNumberProperty(props, SDL_WINDOW_METAL, 0);   // Disable Metal for this example
    //SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FLAG_CHILD, 1);   // Make this a child window
    
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
    
    // Clear the renderer
    SDL_SetRenderDrawColor(sdlRenderer, 64, 64, 64, 255);
    SDL_RenderClear(sdlRenderer);
    
    // Draw a rotating rectangle
    SDL_FRect rect;
    rect.w = 100;
    rect.h = 100;
    rect.x = width() / 2.0f - rect.w / 2.0f;
    rect.y = height() / 2.0f - rect.h / 2.0f;
    
    // Set rotation center
    SDL_FPoint center;
    center.x = rect.x + rect.w / 2.0f;
    center.y = rect.y + rect.h / 2.0f;
    
    // Draw the rotating rectangle
    SDL_SetRenderDrawColor(sdlRenderer, 255, 128, 0, 255);
    
    // Create a transformed renderer
    /*
    SDL_RenderTransformData transform;
    SDL_memset(&transform, 0, sizeof(transform));
    transform.rotation_pivot = &center;
    transform.rotation_degrees = angle;
    
    SDL_RenderWithTransform(sdlRenderer, &transform, [](SDL_Renderer* renderer, void* userdata) {
        SDL_FRect* rect = static_cast<SDL_FRect*>(userdata);
        SDL_RenderFillRect(renderer, rect);
        return 0;
    }, &rect);*/
    
    // Draw a border around the widget
    SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 255, 255);
    SDL_FRect border = {0, 0, static_cast<float>(width()), static_cast<float>(height())};
    SDL_RenderRect(sdlRenderer, &border);
    
    // Present the renderer
    SDL_RenderPresent(sdlRenderer);
    
    // Update the angle for next frame
    angle += 1.0;
    if (angle >= 360.0) {
        angle -= 360.0;
    }
}
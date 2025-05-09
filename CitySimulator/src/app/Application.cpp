#include "stdafx.h"

#include "Application.h"

#include "render/RenderBase.h"

#include "uiSystem/UISystem.h"
#include "render/RenderBase.h"
#include "visualization/SceneSystem.h"

#include <core/dataLayer/WorldData.h>


namespace tjs {

    void FrameStats::setFPS(float fps, FrameStats::duration frameTime) {
        static constexpr float smoothingFactor = 0.005f;
        _fps = fps;
        _frameTime = frameTime;
        
        _smoothedFPS = _smoothedFPS * (1.0 - smoothingFactor) + _fps * smoothingFactor;
    }


    Application::Application(int& argc, char** argv, ApplicationConfig&& config)
        : _commandLine(argc, argv)
        , _config(config)
        , _frameStats(_config.targetFPS)
    {
    }

    void Application::setup(
            std::unique_ptr<IRenderer>&& renderer,
            std::unique_ptr<UISystem>&& uiSystem,
            std::unique_ptr<visualization::SceneSystem>&& sceneSystem,
            std::unique_ptr<core::WorldData>&& worldData
    ) {
        _renderer = std::move(renderer);
        _uiSystem = std::move(uiSystem);
        _sceneSystem = std::move(sceneSystem);
        _worldData = std::move(worldData);

        _settings.load();
    }

    void Application::initialize() {
        _uiSystem->initialize();
        _renderer->initialize();
        _sceneSystem->initialize();
    }

    void Application::run() {
        using duration = FrameStats::duration;

        const duration targetFrameTime(1.0 / _config.targetFPS);

        auto lastFrameTime = std::chrono::high_resolution_clock::now();
        int currentFPS = 0.0;

        auto lastTimeSaveSettings = std::chrono::high_resolution_clock::now();

        while (!isFinished()) {
            // Record the start time of this frame
            auto frameStart = std::chrono::high_resolution_clock::now();
            
            // Run the update and draw operations
            _uiSystem->update();
            _renderer->update();
            _sceneSystem->update();

            // Rendering
            _renderer->beginFrame();
            _sceneSystem->render(*_renderer);
            _renderer->endFrame();

            // Calculate actual frame time
            auto frameEnd = std::chrono::high_resolution_clock::now();

            // check for saving settings
            duration saveDuration = frameEnd - lastTimeSaveSettings;
            if (saveDuration.count() > 10) {
                _settings.save();
                lastTimeSaveSettings = frameEnd;
            }

            duration frameDuration = frameEnd - frameStart;
            duration totalFrameTime = frameEnd - lastFrameTime;
            lastFrameTime = frameEnd;
            
            // Calculate current FPS
            float fps = 0.0f;
            if (totalFrameTime.count() > 0) {
                fps = 1.0f / totalFrameTime.count();
            }
            _frameStats.setFPS(fps, frameDuration);

            // Calculate how long to sleep to maintain target FPS
            duration sleepTime = targetFrameTime - frameDuration;
            // If we're running faster than the target FPS, sleep for the remaining time
            if (sleepTime.count() > 0) {
                std::this_thread::sleep_for(std::chrono::duration_cast<std::chrono::milliseconds>(sleepTime));
            }
        }
    
        // Save settings before quit
        _settings.save();
    }

}

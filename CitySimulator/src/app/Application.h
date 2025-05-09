#pragma once

#include "settings/UserSettings.h"

namespace tjs {

    class UISystem;
    class IRenderer;
    namespace visualization {
        class SceneSystem;
    }

    namespace core {
        class WorldData;
    }

    class CommandLine {
    public:
        CommandLine(int& argc, char** argv)
            : _argc(argc)
            , _argv(argv)
        {}

        int& argc() {
            return _argc;
        }

        char** argv() {
            return _argv;
        }
            
    private:
        int _argc = 0;
        char** _argv = nullptr;
    };

    
    struct FrameStats {
        using duration = std::chrono::duration<float>;

        void init(float fps) {
            _smoothedFPS = fps;
            _fps = fps;
        }
        
        void setFPS(float fps, duration frameTime);
        float smoothedFPS() const {
            return _smoothedFPS;
        }
        float currentFPS() const {
            return _fps;
        }
        duration frameTime() const {
            return _frameTime;
        }
        
        private:
            float _smoothedFPS = 60.0;
            float _fps = 0.f;
            duration _frameTime {0};
    };


    class Application {
    public:
        Application(int& argc, char** argv);
        
        void setFinished() {
            _isFinished = true;
        }

        bool isFinished() const {
            return _isFinished;
        }

        const FrameStats& frameStats() const {
            return _frameStats;
        }

        CommandLine& commandLine() {
            return _commandLine;
        }

        void setup(
            std::unique_ptr<IRenderer>&& renderer,
            std::unique_ptr<UISystem>&& uiSystem,
            std::unique_ptr<visualization::SceneSystem>&& sceneSystem,
            std::unique_ptr<core::WorldData>&& worldData
        );
        void initialize();
        void run();

        // System getters
        UserSettings& settings() {
            return _settings;
        }

        core::WorldData& worldData() {
            return *_worldData;
        }

        UISystem& uiSystem() {
            return *_uiSystem;
        }

        IRenderer& renderer() {
            return *_renderer;
        }

        visualization::SceneSystem& sceneSystem() {
            return *_sceneSystem;
        }

    private:
        CommandLine _commandLine;
        bool _isFinished = false;
        
        FrameStats _frameStats;

        UserSettings _settings;

        // Systems
        std::unique_ptr<IRenderer> _renderer;
        std::unique_ptr<UISystem> _uiSystem;
        std::unique_ptr<visualization::SceneSystem> _sceneSystem;
        std::unique_ptr<core::WorldData> _worldData;
    };
}
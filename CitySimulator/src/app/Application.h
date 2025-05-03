#pragma once

namespace tjs {

    class UISystem;
    class IRenderer;

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


    struct ApplicationConfig {
        int targetFPS = 60;
    };
    
    struct FrameStats {
        using duration = std::chrono::duration<float>;

        FrameStats(float targetFPS)
            : _smoothedFPS(targetFPS) {

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
        Application(int& argc, char** argv, ApplicationConfig&& config);
        
        void setFinished() {
            _isFinished = true;
        }

        bool isFinished() const {
            return _isFinished;
        }

        const FrameStats& frameStats() const {
            return _frameStats;
        }

        CommandLine& getCommandLine() {
            return _commandLine;
        }

        void setup(
            std::unique_ptr<IRenderer>&& renderer,
            std::unique_ptr<UISystem>&& uiSystem
        );
        void initialize();
        void run();
    private:
        ApplicationConfig _config;
        CommandLine _commandLine;
        bool _isFinished = false;
        
        FrameStats _frameStats;

        // Systems
        std::unique_ptr<IRenderer> _renderer;
        std::unique_ptr<UISystem> _uiSystem;
    };
}
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

    class Application {
    public:
        Application(int& argc, char** argv);
        
        void setFinished() {
            _isFinished = true;
        }

        bool isFinished() const {
            return _isFinished;
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
        CommandLine _commandLine;

        bool _isFinished = false;
        std::unique_ptr<IRenderer> _renderer;
        std::unique_ptr<UISystem> _uiSystem;
    };
}
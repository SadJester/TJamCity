#pragma once

namespace tjs::simulation {
    struct TimeState {
        double simulationTime = 0.0;
        double unscaledsimulationTime = 0.0f;

        double timeDelta = 0.0f;
        double unscaledTimeDelta = 0.0f;

        double timeMultiplier = 1.0;
        bool isPaused = false;
        bool stepRequested = false;
    };

    class TimeModule {
    public:
        static constexpr double DEFAULT_STEP_TIME = 0.016;
        // 5 km with 60 km/h will take 5 min
        // to make it pass on the screen for 10 seconds time should be scaled x30
        static constexpr double DEFAULT_TIME_MULTIPLIER = 30;

    public:
        TimeModule(double stepDelta = TimeModule::DEFAULT_STEP_TIME);

        void update(double realTimeDelta);
        
        void speed_up();
        void slow_down();

        const TimeState& state() const;
        
        void pause();
        void resume();
        void step();

    private:
        TimeState _timeState;
        double _stepDelta = 0.016;
    };
}

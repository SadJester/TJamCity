#include "stdafx.h"

#include "Application.h"

#include "render/render_base.h"

#include "ui_system/ui_system.h"
#include "render/render_base.h"
#include "visualization/scene_system.h"

#include <core/data_layer/world_data.h>
#include <core/simulation/simulation_system.h>

namespace tjs {
	Application::Application(int& argc, char** argv)
		: _commandLine(argc, argv) {
	}

	void Application::load_settings() {
		_settings.load();
	}

	void Application::setup(
		std::unique_ptr<IRenderer>&& renderer,
		std::unique_ptr<UISystem>&& uiSystem,
		std::unique_ptr<visualization::SceneSystem>&& sceneSystem,
		std::unique_ptr<core::WorldData>&& worldData,
		std::unique_ptr<core::simulation::TrafficSimulationSystem>&& simulationSystem) {
		_renderer = std::move(renderer);
		_uiSystem = std::move(uiSystem);
		_sceneSystem = std::move(sceneSystem);
		_worldData = std::move(worldData);
		_simulationSystem = std::move(simulationSystem);

		_frameStats.init(_settings.render.targetFPS);
	}

	void Application::initialize() {
		_uiSystem->initialize();
		_renderer->initialize();
		_sceneSystem->initialize();
		_simulationSystem->initialize();

		_models_store.init();
		_logic_modules.init();
	}

	void Application::run() {
		using duration = FrameStats::duration;

		const int targetFPS = _settings.render.targetFPS;
		const duration targetFrameTime(1.0 / targetFPS);

		auto lastFrameTime = std::chrono::high_resolution_clock::now();
		int currentFPS = 0.0;

		auto lastTimeSaveSettings = lastFrameTime;
		auto prevFrameStart = lastFrameTime;
		while (!isFinished()) {
			TJS_TRACY_NAMED("MainLoop");
			// Record the start time of this frame
			auto frameStart = std::chrono::high_resolution_clock::now();

			// Calculate time elapsed since last simulation update for simulation calculations
			const double durationInSeconds = std::chrono::duration_cast<std::chrono::duration<double>>(
				frameStart - prevFrameStart)
												 .count();
			_simulationSystem->update(durationInSeconds);
			auto simulation_end = std::chrono::high_resolution_clock::now();

			prevFrameStart = frameStart;

			_frameStats.simulation_update().update(
				std::chrono::duration_cast<std::chrono::duration<double>>(simulation_end - frameStart).count());

			// Run the update and draw operations
			_uiSystem->update();
			_renderer->update();
			_sceneSystem->update();

			auto systems_end = std::chrono::high_resolution_clock::now();
			_frameStats.systems_update().update(
				std::chrono::duration_cast<std::chrono::duration<double>>(systems_end - simulation_end).count());

			// Rendering
			_renderer->begin_frame();
			_sceneSystem->render(*_renderer);
			_renderer->end_frame();

			auto rendering_end = std::chrono::high_resolution_clock::now();
			_frameStats.render_time().update(
				std::chrono::duration_cast<std::chrono::duration<double>>(rendering_end - systems_end).count());

			// Calculate actual frame time
			auto frameEnd = rendering_end;

			// check for saving settings
			duration saveDuration = frameEnd - lastTimeSaveSettings;
			if (saveDuration.count() > UserSettings::AUTOSAVE_TIME_SEC) {
				// TODO: Refactor when it will be error handling mechanism
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
			_frameStats.fps().update(fps);
			_frameStats.set_frame_time(frameDuration);

			TJS_FRAME_MARK;
			// Calculate how long to sleep to maintain target FPS
			duration sleepTime = targetFrameTime - frameDuration;
			// If we're running faster than the target FPS, sleep for the remaining time
			if (sleepTime.count() > 0) {
				std::this_thread::sleep_for(std::chrono::duration_cast<std::chrono::milliseconds>(sleepTime));
			}
		}

		// Save settings before quit
		_settings.save();

		_logic_modules.release();
		_models_store.release();
	}

} // namespace tjs

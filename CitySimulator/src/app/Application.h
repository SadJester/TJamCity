#pragma once

#include <settings/user_settings.h>
#include <core/store_models/idata_model.h>
#include <core/utils/smoothed_value.h>
#include <logic/logic_base.h>

#include <common/message_dispatcher/message_dispatcher.h>

namespace tjs {

	class UISystem;
	class IRenderer;

	namespace visualization {
		class SceneSystem;
	} // namespace visualization

	namespace core {
		class WorldData;
		namespace simulation {
			class TrafficSimulationSystem;
		} // namespace simulation
	}     // namespace core

	class CommandLine {
	public:
		CommandLine(int& argc, char** argv)
			: _argc(argc)
			, _argv(argv) {}

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
			_fps.set(fps);
		}

		void set_frame_time(duration frame_time) {
			_frame_time = frame_time;
		}

		SmoothedValue& fps() {
			return _fps;
		}

		SmoothedValue& systems_update() {
			return _systems_update;
		}

		SmoothedValue& simulation_update() {
			return _simulation_update;
		}

		SmoothedValue& render_time() {
			return _render_time;
		}

		const SmoothedValue& fps() const {
			return _fps;
		}

		const SmoothedValue& systems_update() const {
			return _systems_update;
		}

		const SmoothedValue& simulation_update() const {
			return _simulation_update;
		}

		const SmoothedValue& render_time() const {
			return _render_time;
		}

		duration frame_time() const {
			return _frame_time;
		}

	private:
		SmoothedValue _fps { 60.f, 0.05f };
		SmoothedValue _systems_update { 0.f, 0.05f };
		SmoothedValue _simulation_update { 0.f, 0.05f };
		SmoothedValue _render_time { 0.f, 0.05f };
		duration _frame_time { 0 };
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

		void load_settings();
		void setup(
			std::unique_ptr<IRenderer>&& renderer,
			std::unique_ptr<UISystem>&& uiSystem,
			std::unique_ptr<visualization::SceneSystem>&& sceneSystem,
			std::unique_ptr<core::WorldData>&& worldData,
			std::unique_ptr<core::simulation::TrafficSimulationSystem>&& simulationSystem);

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

		core::simulation::TrafficSimulationSystem& simulationSystem() {
			return *_simulationSystem;
		}

		core::model::DataModelStore& stores() {
			return _models_store;
		}

		LogicHandler& logic_modules() {
			return _logic_modules;
		}

		common::MessageDispatcher& message_dispatcher() {
			return _message_dispatcher;
		}

	private:
		CommandLine _commandLine;
		bool _isFinished = false;

		FrameStats _frameStats;

		UserSettings _settings;
		core::model::DataModelStore _models_store;

		common::MessageDispatcher _message_dispatcher;

		// Systems
		std::unique_ptr<IRenderer> _renderer;
		std::unique_ptr<UISystem> _uiSystem;
		std::unique_ptr<visualization::SceneSystem> _sceneSystem;
		std::unique_ptr<core::WorldData> _worldData;
		std::unique_ptr<core::simulation::TrafficSimulationSystem> _simulationSystem;

		LogicHandler _logic_modules;
	};

	void setup_models(Application& app);
} // namespace tjs

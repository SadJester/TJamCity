#include "stdafx.h"

#include <ui_system/ui_system.h>

#include <Application.h>

#include <ui_system/qt_ui/qt_controller.h>

#include <visual_system/visual_system.h>

namespace tjs {
	void UISystem::initialize() {
		// TODO{threaded}: will move to _initialize_impl
		auto shared = _application.stores().get_entry<core::model::MapRendererShared>();
		_render_data_connection = shared->connect();
	}

	void UISystem::post_init() {
		auto& v_sys = *_application.systems().get<visualization::VisualSystem>();
		_visual_lossy_reader = v_sys.optional_qeueue().connect();
		_visual_lossless_reader = v_sys.mandatory_queue().connect();

		_controller = std::make_unique<ui::QTUIController>(_application);
		_controller->run();
	}

	void UISystem::update() {
		TJS_TRACY_NAMED("UISystem_Update");

		if (!_controller) {
			return;
		}

		_process_events();
		_controller->update();
	}

	void UISystem::_process_events() {
		_visual_lossless_reader.read_all(
			[this](const visualization::VisualSystem::lossless_queue::message_t& msg) {
				_visual_events_bus.publish(msg);
			});

		_visual_lossy_reader.read_all(
			[this](const visualization::VisualSystem::lossy_queue::message_t& msg) {
				_visual_events_bus.publish(msg);
			});
	}

} // namespace tjs

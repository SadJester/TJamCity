#include "stdafx.h"

#include "ui_system/ui_system.h"
#include "ui_system/qt_ui/qt_controller.h"
#include "Application.h"

namespace tjs {
	void UISystem::initialize() {
		// TODO{threaded}: will move to _initialize_impl
		auto shared = _application.stores().get_entry<core::model::MapRendererShared>();
		_render_data_connection = shared->connect();

		_controller = std::make_unique<ui::QTUIController>(_application);
		_controller->run();
	}

	void UISystem::update() {
		TJS_TRACY_NAMED("UISystem_Update");
		_controller->update();
	}
} // namespace tjs

#include "stdafx.h"

#include "ui_system/ui_system.h"
#include "ui_system/qt_ui/qt_controller.h"
#include "Application.h"

namespace tjs {
	void UISystem::initialize() {
		_controller = std::make_unique<ui::QTUIController>(_application);
		_controller->run();
	}

	void UISystem::update() {
		TJS_TRACY_NAMED("UISystem_Update");
		_controller->update();
	}
} // namespace tjs

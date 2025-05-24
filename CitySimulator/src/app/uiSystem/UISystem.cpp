#include "stdafx.h"

#include "uiSystem/UISystem.h"
#include "uiSystem/qtUI/QTController.h"
#include "Application.h"

namespace tjs {
	void UISystem::initialize() {
		_controller = std::make_unique<ui::QTUIController>(_application);
		_controller->run();
	}

	void UISystem::update() {
		_controller->update();
	}
} // namespace tjs

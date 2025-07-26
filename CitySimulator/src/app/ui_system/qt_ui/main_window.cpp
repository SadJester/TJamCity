#include "stdafx.h"

#include "ui_system/qt_ui/main_window.h"

#include "Application.h"

#include <QWidget>
#include <QTimer>

namespace tjs {
	namespace ui {
		MainWindow::MainWindow(Application& app, QWidget* parent, Qt::WindowFlags flags)
			: QMainWindow(parent, flags)
			, _app(app) {
			_updateTimer = new QTimer(this);
			_updateTimer->start(1000 / settings::RenderSettings::DEFAULT_FPS);
		}

		void MainWindow::closeEvent(QCloseEvent* event) {
			auto& win = _app.settings().general.qt_window;
			const QRect geom = geometry();
			win.x = geom.x();
			win.y = geom.y();
			win.width = geom.width();
			win.height = geom.height();
			_app.setFinished();
		}
	} // namespace ui
} // namespace tjs

#include "stdafx.h"

#include "uiSystem/qtUI/MainWindow.h"

#include "Application.h"

#include <QWidget>
#include <QTimer>


namespace tjs {
    namespace ui {
        MainWindow::MainWindow(Application& app, QWidget* parent, Qt::WindowFlags flags)
            : QMainWindow(parent, flags)
            ,  _app(app) {
            _updateTimer = new QTimer(this);
            _updateTimer->start(1000 / settings::RenderSettings::DEFAULT_FPS);
        }
        
        void MainWindow::closeEvent (QCloseEvent *event) {
            _app.setFinished();
        }
    }
}

#include "stdafx.h"

#include "uiSystem/qtUI/QTController.h"

#include "uiSystem/qtUI/MainWindow.h"

#include "Application.h"

#include <QApplication>
#include <QPushButton>
#include <QVBoxLayout>

/// TODO: Place somwhere to be more pretty
#include "uiSystem/qtUI/RenderMetricsWidget.h"
#include "uiSystem/qtUI/MapControlWidget.h"


namespace tjs {
    namespace ui {
         QTUIController::QTUIController(Application& application) 
            : _application(application)
        {
        }
                
        QTUIController::~QTUIController() {
            // Request application to quit
            if (m_app) {
                m_app->quit();
            }
        }

        void QTUIController::run() {
            m_app = std::make_unique<QApplication>(
                _application.commandLine().argc(),
                _application.commandLine().argv()
            );
            createAndShowMainWindow();
        }

        void QTUIController::update() {
            m_app->processEvents();
        }

        void QTUIController::createAndShowMainWindow() {
            // Create main window
            MainWindow* window = new MainWindow(_application);
            window->setWindowTitle("TJS");
            window->resize(400, 800);
            
            // Create central widget and layout
            QWidget* centralWidget = new QWidget(window);
            QVBoxLayout* layout = new QVBoxLayout(centralWidget);
            
            RenderMetricsWidget* fpsLabel = new RenderMetricsWidget(_application, window);
            layout->addWidget(fpsLabel);
            
            MapControlWidget* mapControlWidget = new MapControlWidget(_application, window);
            layout->addWidget(mapControlWidget);

            // Add widgets
            QPushButton* button = new QPushButton("Quit");
            QObject::connect(button, &QPushButton::clicked, [this]() {
                _application.setFinished();
            });
            layout->addWidget(button);
            
            window->setCentralWidget(centralWidget);
            window->show();
        }
    }
}


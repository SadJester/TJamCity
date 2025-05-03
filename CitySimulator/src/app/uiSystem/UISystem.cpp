#include "uiSystem/UISystem.h"

#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMetaObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QTimer>
#include <QDebug>
#include <thread>
#include <iostream>
#include <atomic>

#include "Application.h"


namespace tjs {
    namespace details
    {
        class RenderMetricsWidget : public QWidget {
            //Q_OBJECT

        private:
            QLabel* fpsLabel;
            QTimer* timer;
            Application& _app;

        public:
            RenderMetricsWidget(Application& app, QWidget* parent = nullptr) 
                : QWidget(parent)
                , _app(app) {
                // Create a central widget and layout
                QWidget* centralWidget = new QWidget(this);
                QVBoxLayout* layout = new QVBoxLayout(centralWidget);
                
                // Create FPS label
                fpsLabel = new QLabel("FPS: 00 | Frame time: 00 ms", this);
                fpsLabel->setAlignment(Qt::AlignCenter);
                fpsLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
                fpsLabel->setStyleSheet("font-size: 24px; font-weight: bold;");
                
                layout->addWidget(fpsLabel);
                
                // Set up timer for frame updates
                timer = new QTimer(this);
                connect(timer, &QTimer::timeout, this, &RenderMetricsWidget::updateFrame);
                timer->start(16); // ~60 FPS target (16ms)
            }

        private slots:
            void updateFrame() {
                // Update FPS label
                auto& stats = _app.frameStats();
                fpsLabel->setText(
                    QString("FPS: %1 (%2 ms)")
                        .arg(stats.smoothedFPS(), 2, 'f', 2)
                        .arg(std::chrono::duration_cast<std::chrono::milliseconds>(stats.frameTime()).count())
                );

                if (stats.currentFPS() < 30.f) {
                    fpsLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: red;");
                } else if (stats.currentFPS() < 50.0f) {
                    fpsLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: orange;");
                } else {
                    fpsLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: green;");
                }

                update();
            }
        };


        class MainWindow final : public QMainWindow {
            public:
                MainWindow(Application& app, QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags())
                    : QMainWindow(parent, flags)
                    ,  _app(app) {

                }

                virtual void closeEvent (QCloseEvent *event) override {
                    _app.setFinished();
                }
            private:
                Application& _app;
        };


        class QTUIController final
            : public IUIController
        {
            public:
                // Constructor takes argc and argv for QApplication
                QTUIController(Application& application) 
                    : _application(application)
                {
                }
                
                ~QTUIController() {
                    // Request application to quit
                    if (m_app) {
                        m_app->quit();
                    }
                }

                virtual void run() override {
                    m_app = std::make_unique<QApplication>(
                        _application.getCommandLine().argc(),
                        _application.getCommandLine().argv()
                    );
                    createAndShowMainWindow();
                }

                virtual void update() override {
                    m_app->processEvents();
                }

            private:
                void createAndShowMainWindow() {
                    // Create main window
                    MainWindow* window = new MainWindow(_application);
                    window->setWindowTitle("TJS");
                    window->resize(400, 800);
                    
                    // Create central widget and layout
                    QWidget* centralWidget = new QWidget(window);
                    QVBoxLayout* layout = new QVBoxLayout(centralWidget);
                    
                    RenderMetricsWidget* fpsLabel = new RenderMetricsWidget(_application);
                    layout->addWidget(fpsLabel);

                    // Add widgets
                    QPushButton* button = new QPushButton("Quit");
                    QObject::connect(button, &QPushButton::clicked, [this]() {
                        _application.setFinished();
                    });
                    layout->addWidget(button);
                    
                    window->setCentralWidget(centralWidget);
                    window->show();
                }

            private:
                std::unique_ptr<QApplication> m_app = nullptr;
                Application& _application;
                bool m_appInitialized = false;
        };
    }
    
    void UISystem::initialize() {
        auto cmdLine = _application.getCommandLine();

        _controller = std::make_unique<details::QTUIController>(_application);
        _controller->run();
    }

    void UISystem::update() {
        _controller->update();
    }
}

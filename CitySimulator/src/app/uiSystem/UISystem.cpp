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
        class QTUIController final
            : public IUIController
        {
            public:
                // Constructor takes argc and argv for QApplication
                QTUIController(Application& application, int& argc, char** argv) 
                    : _application(application)
                    ,  m_argc(argc)
                    , m_argv(argv)
                {
                }
                
                ~QTUIController() {
                    // Request application to quit
                    if (m_app) {
                        m_app->quit();
                    }
                }

                virtual void run() override {
                    int argc = m_argc;
                    std::cout << "Create app in thread " << std::this_thread::get_id() << std::endl;

                    m_app = std::make_unique<QApplication>(argc, m_argv);
                    createAndShowMainWindow();
                }

                virtual void update() override {
                    m_app->processEvents();
                }

            private:
                void createAndShowMainWindow() {
                    // Create main window
                    QMainWindow* window = new QMainWindow();
                    window->setWindowTitle("Qt App in Separate Thread");
                    window->resize(400, 300);
                    
                    // Create central widget and layout
                    QWidget* centralWidget = new QWidget(window);
                    QVBoxLayout* layout = new QVBoxLayout(centralWidget);
                    
                    // Add widgets
                    QLabel* label = new QLabel("Application running in separate thread");
                    layout->addWidget(label);
                    
                    QPushButton* button = new QPushButton("Click Me");
                    QObject::connect(button, &QPushButton::clicked, [label]() {
                        label->setText("Button clicked in separate thread!");
                    });
                    layout->addWidget(button);


                    button = new QPushButton("Quit");
                    QObject::connect(button, &QPushButton::clicked, [label, this]() {
                        _application.setFinished();
                    });
                    layout->addWidget(button);
                    
                    window->setCentralWidget(centralWidget);
                    window->show();
                }

            private:
                int m_argc;
                char** m_argv;
                std::unique_ptr<QApplication> m_app = nullptr;
                Application& _application;
                bool m_appInitialized = false;
        };
    }
    
    void UISystem::initialize(int& argc, char** argv) {
        _controller = std::make_unique<details::QTUIController>(_application, argc, argv);
        _controller->run();
    }

    void UISystem::update() {
        _controller->update();
    }
}

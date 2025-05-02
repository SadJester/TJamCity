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

#include <memory>

namespace tjs {
    class Application;

    class IUIController {
        public:
            virtual ~IUIController() {}
            virtual void run() = 0;
            virtual void update() = 0;
    };

    class UISystem {
        public:
            UISystem(Application& application)
                : _application(application)
            {
            }

            void initialize(int& argc, char** argv);
            void update();

        private:
            std::unique_ptr<IUIController> _controller;
            Application& _application;
    };


    // Custom thread class to run the Qt event loop
    class QtAppThread : public QThread {
    public:
        // Constructor takes argc and argv for QApplication
        QtAppThread(int& argc, char** argv) : m_argc(argc), m_argv(argv), m_app(nullptr) {}
        
        ~QtAppThread() {
            // Request application to quit
            if (m_app) {
                m_app->quit();
                wait(); // Wait for the thread to finish
            }
        }

        // Method to access the application pointer from outside
        QApplication* application() const {
            QMutexLocker locker(&m_mutex);
            return m_app;
        }

        // Method to wait until the application is started
        void waitForInit() {
            if (!m_appInitialized) {
                QMutexLocker locker(&m_mutex);
                if (!m_appInitialized) {
                    m_waitCondition.wait(&m_mutex);
                }
            }
        }

    protected:
        void run() override {
            // Create QApplication in this thread
            int argc = m_argc;
            QApplication app(argc, m_argv);
            
            {
                QMutexLocker locker(&m_mutex);
                m_app = &app;
                m_appInitialized = true;
                m_waitCondition.wakeAll(); // Signal that app is initialized
            }
            
            // Create and show the main window in this thread
            createAndShowMainWindow();
            
            // Start the event loop
            app.exec();
            
            // Cleanup when event loop ends
            {
                QMutexLocker locker(&m_mutex);
                m_app = nullptr;
                m_appInitialized = false;
            }
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
            
            window->setCentralWidget(centralWidget);
            window->show();
        }

        int m_argc;
        char** m_argv;
        QApplication* m_app;
        mutable QMutex m_mutex;
        QWaitCondition m_waitCondition;
        bool m_appInitialized = false;
    };

    // Class to handle cross-thread communication
    class QtThreadComm : public QObject {
        //Q_OBJECT
    public:
        explicit QtThreadComm(QObject* parent = nullptr) : QObject(parent) {}

    public slots:
        void showMessage(const QString& message) {
            qDebug() << "Message from main thread: " << message;
        }

    signals:
        void messageToMainThread(const QString& message);
    };

}
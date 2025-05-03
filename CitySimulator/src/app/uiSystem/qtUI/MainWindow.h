#pragma once

#include <QMainWindow>

class QTimer;

namespace tjs {
    class Application;

    namespace ui {
        class MainWindow final : public QMainWindow {
            public:
                MainWindow(Application& app, QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

                QTimer* timer() {
                    return _updateTimer;
                }

                virtual void closeEvent (QCloseEvent *event) override;

            private:
                Application& _app;
                QTimer* _updateTimer;
        };


    }
}

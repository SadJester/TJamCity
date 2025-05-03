#pragma once

#include <QWidget>
#include <QLabel>


namespace tjs {
    class Application;

    namespace ui {
        class MainWindow;

        class RenderMetricsWidget : public QWidget {
            //Q_OBJECT

        private:
            QLabel* fpsLabel;
            Application& _app;

        public:
            RenderMetricsWidget(Application& app, MainWindow* parent);

        private slots:
            void updateFrame();
        };
    }
}
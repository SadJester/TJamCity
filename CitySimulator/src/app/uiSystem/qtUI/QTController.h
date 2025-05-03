#pragma once

#include "uiSystem/UISystem.h"

class QApplication;

namespace tjs {
    class Application;

    namespace ui
    {
        class QTUIController final
            : public IUIController
        {
            public:
                QTUIController(Application& application);
                ~QTUIController();

                virtual void run() override;
                virtual void update() override;
            private:
                void createAndShowMainWindow();

            private:
                std::unique_ptr<QApplication> m_app;
                Application& _application;
                bool m_appInitialized = false;
        };
    }
}

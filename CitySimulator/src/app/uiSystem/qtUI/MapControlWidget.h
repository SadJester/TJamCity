#pragma once


#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QLabel>


namespace tjs {
    class Application;

    namespace visualization {
        class MapElement;
    }

    namespace ui {
        class MapControlWidget : public QWidget {
            // Q_OBJECT
        
        public:
            explicit MapControlWidget(Application& application, QWidget* parent = nullptr);
            ~MapControlWidget() override;
        
        private:
            void UpdateButtonsState();
            void UpdateLabels();

        private slots:
            void onZoomIn();
            void onZoomOut();
            void onLatitudeChanged(double value);
            void onLongitudeChanged(double value);
            void moveNorth();
            void moveSouth();
            void moveWest();
            void moveEast();
            void onUpdate();
            void openOSMFile();
        private:
            Application& _application;
            visualization::MapElement* _mapElement = nullptr;
            // Temporary button, will erase it after refactoring
            QPushButton* _updateButton;

            QPushButton* _zoomInButton;
            QPushButton* _zoomOutButton;
            QDoubleSpinBox* _latitude;
            QDoubleSpinBox* _longitude;
            QPushButton* _northButton;
            QPushButton* _southButton;
            QPushButton* _westButton;
            QPushButton* _eastButton;
            QPushButton* _openFileButton;

            QLabel* _zoomLevel;
        };
    }
}

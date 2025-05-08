#include "stdafx.h"

#include "uiSystem/qtUI/MapControlWidget.h"

#include "Application.h"
#include "visualization/elements/MapElement.h"

#include <QLabel>


/// Place somwhere to be more pretty
#include "visualization/Scene.h"
#include "visualization/SceneSystem.h"
#include "visualization/elements/MapElement.h"


namespace tjs {
    namespace ui {

        static constexpr double CHANGE_CENTER_STEP = 0.001;

        MapControlWidget::MapControlWidget(Application& application, QWidget* parent)
            : QWidget(parent)
            , _application(application) {
             // Create main layout
            QVBoxLayout* mainLayout = new QVBoxLayout(this);

            // Update button
            _updateButton = new QPushButton("Update");
            mainLayout->addWidget(_updateButton);
            connect(_updateButton, &QPushButton::clicked, this, &MapControlWidget::onUpdate);

            // Create zoom buttons layout
            QHBoxLayout* zoomLayout = new QHBoxLayout();
            _zoomInButton = new QPushButton("Zoom In", this);
            _zoomOutButton = new QPushButton("Zoom Out", this);
            zoomLayout->addWidget(_zoomInButton);
            zoomLayout->addWidget(_zoomOutButton);
            
            // Create arrows layout
            QFrame* arrowsFrame = new QFrame();
            arrowsFrame->setFrameStyle(QFrame::Box | QFrame::Sunken);
            arrowsFrame->setLineWidth(2);
            arrowsFrame->setMidLineWidth(1);
            
            QGridLayout* arrowsLayout = new QGridLayout(arrowsFrame);
            
            _northButton = new QPushButton("▲");
            _westButton = new QPushButton("◀");
            _eastButton = new QPushButton("▶");
            _southButton = new QPushButton("▼");
            
            arrowsLayout->addWidget(_northButton, 0, 1);
            arrowsLayout->addWidget(_westButton, 1, 0);
            arrowsLayout->addWidget(_eastButton, 1, 2);
            arrowsLayout->addWidget(_southButton, 2, 1);
            
            // Create coordinates layout
            QHBoxLayout* coordsLayout = new QHBoxLayout();
            QLabel* latLabel = new QLabel("Latitude:");
            QLabel* lonLabel = new QLabel("Longitude:");
            
            _latitude = new QDoubleSpinBox();
            _latitude->setRange(-90.0, 90.0);
            _latitude->setSuffix("°");
            
            _longitude = new QDoubleSpinBox();
            _longitude->setRange(-180.0, 180.0);
            _longitude->setSuffix("°");
            
            coordsLayout->addWidget(latLabel);
            coordsLayout->addWidget(_latitude);
            coordsLayout->addWidget(lonLabel);
            coordsLayout->addWidget(_longitude);
            
            // Add all elements to main layout
            mainLayout->addLayout(zoomLayout);
            mainLayout->addWidget(arrowsFrame);
            mainLayout->addLayout(coordsLayout);
            
            // Connections
            connect(_zoomInButton, &QPushButton::clicked, this, &MapControlWidget::onZoomIn);
            connect(_zoomOutButton, &QPushButton::clicked, this, &MapControlWidget::onZoomOut);
            connect(_northButton, &QPushButton::clicked, this, &MapControlWidget::moveNorth);
            connect(_southButton, &QPushButton::clicked, this, &MapControlWidget::moveSouth);
            connect(_westButton, &QPushButton::clicked, this, &MapControlWidget::moveWest);
            connect(_eastButton, &QPushButton::clicked, this, &MapControlWidget::moveEast);
            connect(_latitude, SIGNAL(valueChanged(double)), this, SLOT(onLatitudeChanged(double)));
            connect(_longitude, SIGNAL(valueChanged(double)), this, SLOT(onLongitudeChanged(double)));

            UpdateButtonsState();
        }

        MapControlWidget::~MapControlWidget() {
            // Cleanup
        }

        void MapControlWidget::UpdateButtonsState() {
            const bool value = _mapElement != nullptr;
            _zoomInButton->setEnabled(value);
            _zoomOutButton->setEnabled(value);
            _northButton->setEnabled(value);
            _southButton->setEnabled(value);
            _westButton->setEnabled(value);
            _eastButton->setEnabled(value);
            _latitude->setEnabled(value);
            _longitude->setEnabled(value);
        }

        void MapControlWidget::onZoomIn() {
            double currentZoom = _mapElement->getZoomLevel();
            _mapElement->setZoomLevel(currentZoom * 0.9); // Zoom in (decrease meters per pixel)
        }
        
        void MapControlWidget::onZoomOut() {
            double currentZoom = _mapElement->getZoomLevel();
            _mapElement->setZoomLevel(currentZoom * 1.1); // Zoom out (increase meters per pixel)
        }
        
        void MapControlWidget::onLatitudeChanged(double value) {
            core::Coordinates newCenter = _mapElement->getProjectionCenter();
            newCenter.latitude = value;
            _mapElement->setProjectionCenter(newCenter);
        }
        
        void MapControlWidget::onLongitudeChanged(double value) {
            core::Coordinates newCenter = _mapElement->getProjectionCenter();
            newCenter.longitude = value;
            _mapElement->setProjectionCenter(newCenter);
        }
        
        void MapControlWidget::moveNorth() {
            core::Coordinates current = _mapElement->getProjectionCenter();
            if (current.latitude < 90.0) {
                current.latitude += CHANGE_CENTER_STEP; // Change step size as needed
                _latitude->setValue(current.latitude);
                _mapElement->setProjectionCenter(current);
            }
        }
        
        void MapControlWidget::moveSouth() {
            core::Coordinates current = _mapElement->getProjectionCenter();
            if (current.latitude > -90.0) {
                current.latitude -= CHANGE_CENTER_STEP; // Change step size as needed
                _latitude->setValue(current.latitude);
                _mapElement->setProjectionCenter(current);
            }
        }
        
        void MapControlWidget::moveWest() {
            core::Coordinates current = _mapElement->getProjectionCenter();
            if (current.longitude > -180.0) {
                current.longitude -= CHANGE_CENTER_STEP; // Change step size as needed
                _longitude->setValue(current.longitude);
                _mapElement->setProjectionCenter(current);
            }
        }
        
        void MapControlWidget::moveEast() {
            core::Coordinates current = _mapElement->getProjectionCenter();
            if (current.longitude < 180.0) {
                current.longitude += CHANGE_CENTER_STEP; // Change step size as needed
                _longitude->setValue(current.longitude);
                _mapElement->setProjectionCenter(current);
            }
        }

        void MapControlWidget::onUpdate() {
            auto scene = _application.sceneSystem().getScene("General");
            if (scene == nullptr) {
                 return;
            }

            _mapElement = dynamic_cast<visualization::MapElement*>(scene->getNode("MapElement"));
            if (_mapElement == nullptr) {
                UpdateButtonsState();
                return;
            }
            UpdateButtonsState();

            // Initialize spin boxes with current values
            const auto& center = _mapElement->getProjectionCenter();
            _latitude->setValue(center.latitude);
            _longitude->setValue(center.longitude);
        }
    }
}

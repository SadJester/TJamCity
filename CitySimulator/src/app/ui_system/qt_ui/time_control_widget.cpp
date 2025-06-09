#include "stdafx.h"

#include "ui_system/qt_ui/time_control_widget.h"
#include "Application.h"
#include "ui_system/qt_ui/main_window.h"

#include <QTimer>
#include <QWidget>
#include <QHBoxLayout>

#include <core/simulation/simulation_system.h>

namespace tjs {
	namespace ui {
		TimeControlWidget::TimeControlWidget(Application& application, QWidget* parent)
			: QWidget(parent)
			, _application(application) {
			// Create main layout
			QHBoxLayout* layout = new QHBoxLayout(this);

			// Create start/pause button
			_startPauseButton = new QPushButton("Start", this);
			connect(_startPauseButton, &QPushButton::clicked, this, &TimeControlWidget::onStartPauseClicked);
			_isRunning = !_application.simulationSystem().timeModule().state().isPaused;
			_startPauseButton->setText(_isRunning ? "Pause" : "Start");
			layout->addWidget(_startPauseButton);

			// Create step button
			_stepButton = new QPushButton("Step", this);
			connect(_stepButton, &QPushButton::clicked, this, &TimeControlWidget::onStepClicked);
			layout->addWidget(_stepButton);

			// Create speed control buttons
			_speedUpButton = new QPushButton("Speed Up", this);
			connect(_speedUpButton, &QPushButton::clicked, this, &TimeControlWidget::onSpeedUpClicked);
			layout->addWidget(_speedUpButton);

			_slowDownButton = new QPushButton("Slow Down", this);
			connect(_slowDownButton, &QPushButton::clicked, this, &TimeControlWidget::onSlowDownClicked);
			layout->addWidget(_slowDownButton);

			// Create time label
			_timeLabel = new QLabel("Time: 0.0x", this);
			_timeLabel->setAlignment(Qt::AlignCenter);
			_timeLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
			_timeLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
			layout->addWidget(_timeLabel);

			// Update time label periodically
			QTimer* timer = new QTimer(this);
			connect(timer, &QTimer::timeout, this, &TimeControlWidget::updateTimeLabel);
			timer->start(100); // Update every 100ms

			updateButtonStates();
		}

		void TimeControlWidget::onStartPauseClicked() {
			if (_isRunning) {
				_application.simulationSystem().timeModule().pause();
				_startPauseButton->setText("Start");
			} else {
				_application.simulationSystem().timeModule().resume();
				_startPauseButton->setText("Pause");
			}
			_isRunning = !_isRunning;
			updateButtonStates();
		}

		void TimeControlWidget::onSpeedUpClicked() {
			_application.simulationSystem().timeModule().speed_up();
			updateTimeLabel();
		}

		void TimeControlWidget::onSlowDownClicked() {
			_application.simulationSystem().timeModule().slow_down();
			updateTimeLabel();
		}

		void TimeControlWidget::onStepClicked() {
			_application.simulationSystem().timeModule().step();
			updateTimeLabel();
		}

		void TimeControlWidget::updateTimeLabel() {
			const auto& timeState = _application.simulationSystem().timeModule().state();
			_timeLabel->setText(QString("Time: %1x").arg(timeState.timeMultiplier, 0, 'f', 1));
		}

		void TimeControlWidget::updateButtonStates() {
			_speedUpButton->setEnabled(_isRunning);
			_slowDownButton->setEnabled(_isRunning);
			_stepButton->setEnabled(!_isRunning); // Step only enabled when paused
		}
	} // namespace ui
} // namespace tjs

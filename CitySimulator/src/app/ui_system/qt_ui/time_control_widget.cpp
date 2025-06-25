#include "stdafx.h"

#include "ui_system/qt_ui/time_control_widget.h"
#include "Application.h"
#include "ui_system/qt_ui/main_window.h"

#include <QTimer>
#include <QWidget>
#include <QHBoxLayout>
#include <QDoubleSpinBox>

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

			// Create speed control spin box
			_speedSpinBox = new QDoubleSpinBox(this);
			_speedSpinBox->setRange(0.1, 100.0);
			_speedSpinBox->setSingleStep(0.5);
			_speedSpinBox->setValue(_application.simulationSystem().timeModule().state().timeMultiplier);
			connect(_speedSpinBox, &QDoubleSpinBox::editingFinished, [this]() {
				onMultiplierChanged(_speedSpinBox->value());
			});
			layout->addWidget(_speedSpinBox);

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

		void TimeControlWidget::onMultiplierChanged(double value) {
			_application.simulationSystem().timeModule().set_time_multiplier(value);
			updateTimeLabel();
		}

		void TimeControlWidget::onStepClicked() {
			_application.simulationSystem().step();
			updateTimeLabel();
		}

		void TimeControlWidget::updateTimeLabel() {
			const auto& timeState = _application.simulationSystem().timeModule().state();
			_timeLabel->setText(QString("Time: %1x").arg(timeState.timeMultiplier, 0, 'f', 1));
		}

		void TimeControlWidget::updateButtonStates() {
			_stepButton->setEnabled(!_isRunning); // Step only enabled when paused
		}
	} // namespace ui
} // namespace tjs

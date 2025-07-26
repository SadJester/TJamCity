#include "stdafx.h"

#include "ui_system/qt_ui/time_control_widget.h"
#include "Application.h"
#include "ui_system/qt_ui/main_window.h"

#include <QTimer>
#include <QWidget>
#include <QHBoxLayout>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLabel>

#include <core/simulation/simulation_system.h>
#include <ctime>
#include <iomanip>
#include <sstream>

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
			_isRunning = !_application.simulationSystem().timeModule().state().isPaused && !_application.simulationSystem().settings().simulation_paused;

			_startPauseButton->setText(_isRunning ? "Pause" : "Start");
			layout->addWidget(_startPauseButton);

			// Create step button
			_stepButton = new QPushButton("Step", this);
			connect(_stepButton, &QPushButton::clicked, this, &TimeControlWidget::onStepClicked);
			layout->addWidget(_stepButton);

			// Create step delta spin box (replacing speed multiplier)
			QLabel* stepDeltaLabel = new QLabel("Step Delta (s):", this);
			stepDeltaLabel->setToolTip("Time step in seconds for simulation");
			layout->addWidget(stepDeltaLabel);

			_stepDeltaSpinBox = new QDoubleSpinBox(this);
			_stepDeltaSpinBox->setRange(0.016, 10.0);
			_stepDeltaSpinBox->setSingleStep(0.05);
			_stepDeltaSpinBox->setDecimals(3);
			_stepDeltaSpinBox->setValue(_application.settings().simulationSettings.step_delta_sec);
			_stepDeltaSpinBox->setToolTip("Time step in seconds for simulation");
			connect(_stepDeltaSpinBox, &QDoubleSpinBox::editingFinished, [this]() {
				onStepDeltaChanged(_stepDeltaSpinBox->value());
			});
			layout->addWidget(_stepDeltaSpinBox);

			// Create steps on update spin box
			QLabel* stepsLabel = new QLabel("Steps/Frame:", this);
			stepsLabel->setToolTip("Number of simulation steps per frame when running");
			layout->addWidget(stepsLabel);

			_steps_on_update = new QSpinBox(this);
			_steps_on_update->setRange(1, 1000);
			_steps_on_update->setValue(_application.settings().simulationSettings.steps_on_update);
			_steps_on_update->setToolTip(stepsLabel->toolTip());
			connect(_steps_on_update, &QSpinBox::valueChanged, this, &TimeControlWidget::onStepsOnUpdateChanged);
			layout->addWidget(_steps_on_update);

			// Create time label
			_timeLabel = new QLabel("Time: 06:00 13/07/2025", this);
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
			_application.settings().simulationSettings.simulation_paused = !_isRunning;
			updateButtonStates();
		}

		void TimeControlWidget::onStepDeltaChanged(double value) {
			_application.simulationSystem().timeModule().set_step_delta(value);
			_application.settings().simulationSettings.step_delta_sec = value;
			updateTimeLabel();
		}

		void TimeControlWidget::onStepsOnUpdateChanged(int value) {
			_application.settings().simulationSettings.steps_on_update = value;
		}

		void TimeControlWidget::onStepClicked() {
			_application.simulationSystem().step();
			updateTimeLabel();
		}

		std::string format_time(const core::TimeState& time_state) {
			const auto& current_time = time_state.current_time();

			// Convert to integral time point
			auto integral_time = std::chrono::time_point_cast<std::chrono::system_clock::duration>(current_time);

			// Now safely convert to time_t
			std::time_t time_t_value = std::chrono::system_clock::to_time_t(integral_time);
			std::tm tm_buffer;
#ifdef _WIN32
			localtime_s(&tm_buffer, &time_t_value);
			std::tm* tm = &tm_buffer;
#else
			std::tm* tm = localtime_r(&time_t_value, &tm_buffer);
#endif
			// Format as d/m/year/h/m

			char buffer[32];
			snprintf(buffer, sizeof(buffer), "%02d:%02d %02d/%02d/%d",
				tm->tm_hour, tm->tm_min, tm->tm_mday,
				tm->tm_mon + 1, tm->tm_year + 1900);
			return { buffer };
		}

		void TimeControlWidget::updateTimeLabel() {
			const auto& time_state = _application.simulationSystem().timeModule().state();
			_timeLabel->setText(QString("Time: %1").arg(format_time(time_state)));
		}

		void TimeControlWidget::updateButtonStates() {
			_stepButton->setEnabled(!_isRunning); // Step only enabled when paused
		}
	} // namespace ui
} // namespace tjs

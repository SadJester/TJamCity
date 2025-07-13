#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSpinBox>

namespace tjs {
	class Application;

	namespace ui {
		class TimeControlWidget : public QWidget {
			//Q_OBJECT

		public:
			explicit TimeControlWidget(Application& application, QWidget* parent = nullptr);
			~TimeControlWidget() override = default;

		private slots:
			void onStartPauseClicked();
			void onStepDeltaChanged(double value);
			void onStepsOnUpdateChanged(int value);
			void onStepClicked();
			void updateTimeLabel();
			void updateButtonStates();

		private:
			Application& _application;
			QPushButton* _startPauseButton;
			QDoubleSpinBox* _stepDeltaSpinBox;
			QSpinBox* _steps_on_update;
			QPushButton* _stepButton;
			QLabel* _timeLabel;
			bool _isRunning = false;
		};
	} // namespace ui
} // namespace tjs

#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QDoubleSpinBox>

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
			void onMultiplierChanged(double value);
			void onStepClicked();
			void updateTimeLabel();
			void updateButtonStates();

		private:
			Application& _application;
			QPushButton* _startPauseButton;
			QDoubleSpinBox* _speedSpinBox;
			QPushButton* _stepButton;
			QLabel* _timeLabel;
			bool _isRunning = false;
		};
	} // namespace ui
} // namespace tjs

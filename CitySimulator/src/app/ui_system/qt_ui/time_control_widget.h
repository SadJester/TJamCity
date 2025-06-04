#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>

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
			void onSpeedUpClicked();
			void onSlowDownClicked();
			void onStepClicked();
			void updateTimeLabel();
			void updateButtonStates();

		private:
			Application& _application;
			QPushButton* _startPauseButton;
			QPushButton* _speedUpButton;
			QPushButton* _slowDownButton;
			QPushButton* _stepButton;
			QLabel* _timeLabel;
			bool _isRunning = false;
		};
	} // namespace ui
} // namespace tjs

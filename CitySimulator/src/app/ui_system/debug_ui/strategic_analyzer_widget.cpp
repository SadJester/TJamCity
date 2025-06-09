#include "ui_system/debug_ui/strategic_analyzer_widget.h"
#include "Application.h"

#include <QVBoxLayout>
#include <QTimer>

#include <core/simulation/simulation_system.h>
#include <core/simulation/agent/agent_data.h>

namespace tjs::ui {

	StrategicAnalyzerWidget::StrategicAnalyzerWidget(Application& app)
		: QWidget(nullptr)
		, _application(app) {
		QVBoxLayout* layout = new QVBoxLayout(this);
		_list = new QListWidget(this);
		layout->addWidget(_list);

		QTimer* timer = new QTimer(this);
		connect(timer, &QTimer::timeout, this, &StrategicAnalyzerWidget::updateInfo);
		timer->start(500);
	}

	void StrategicAnalyzerWidget::updateInfo() {
		_list->clear();
		const auto& agents = _application.simulationSystem().agents();
		for (const auto& agent : agents) {
			if (agent.stucked) {
				_list->addItem(QString("Agent %1").arg(agent.id));
			}
		}
	}

} // namespace tjs::ui

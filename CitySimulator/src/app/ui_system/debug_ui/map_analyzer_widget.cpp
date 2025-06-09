#include "ui_system/debug_ui/map_analyzer_widget.h"
#include "Application.h"
#include "data/persistent_render_data.h"
#include "data/map_renderer_data.h"
#include "data/simulation_debug_data.h"

#include <QVBoxLayout>
#include <QTimer>

namespace tjs::ui {

	MapAnalyzerWidget::MapAnalyzerWidget(Application& app)
		: QWidget(nullptr)
		, _application(app) {
		QVBoxLayout* layout = new QVBoxLayout(this);
		_nodeId = new QLabel("Node: none", this);
		_coords = new QLabel("Coords: 0, 0", this);
		layout->addWidget(_nodeId);
		layout->addWidget(_coords);

		_networkOnly = new QCheckBox("Network only for selected", this);
		layout->addWidget(_networkOnly);
		connect(_networkOnly, &QCheckBox::stateChanged, this, &MapAnalyzerWidget::onNetworkOnlyChanged);

		QTimer* timer = new QTimer(this);
		connect(timer, &QTimer::timeout, this, &MapAnalyzerWidget::updateInfo);
		timer->start(200);
	}

	void MapAnalyzerWidget::updateInfo() {
		auto* debug = _application.stores().get_model<core::model::SimulationDebugData>();
		if (!debug || !debug->selectedNode) {
			_nodeId->setText("Node: none");
			_coords->setText("Coords: -");
			if (auto* render = _application.stores().get_model<core::model::MapRendererData>(); render) {
				_networkOnly->setChecked(render->networkOnlyForSelected);
			}
			return;
		}

		const auto* node = debug->selectedNode->node;
		_nodeId->setText(QString("Node: %1").arg(node->uid));
		_coords->setText(QString("Coords: %1, %2").arg(node->coordinates.latitude).arg(node->coordinates.longitude));
		if (auto* render = _application.stores().get_model<core::model::MapRendererData>(); render) {
			_networkOnly->setChecked(render->networkOnlyForSelected);
		}
	}

	void MapAnalyzerWidget::onNetworkOnlyChanged(int state) {
		auto* render = _application.stores().get_model<core::model::MapRendererData>();
		if (!render) {
			return;
		}

		bool value = state == Qt::Checked;
		if (render->networkOnlyForSelected != value) {
			render->networkOnlyForSelected = value;
			visualization::recalculate_map_data(_application);
		}
	}

} // namespace tjs::ui

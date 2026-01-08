#include <stdafx.h>

#include <ui_system/debug_ui/map_analyzer_widget.h>

#include <Application.h>

#include <ui_system/ui_system.h>
#include <visual_system/visual_system.h>
#include <visual_system/data/map_renderer_data.h>
#include <core/simulation/simulation_debug.h>
#include <core/data_layer/node.h>

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
		connect(_networkOnly, &QCheckBox::checkStateChanged, this, &MapAnalyzerWidget::onNetworkOnlyChanged);

		QTimer* timer = new QTimer(this);
		connect(timer, &QTimer::timeout, this, &MapAnalyzerWidget::updateInfo);
		timer->start(200);
	}

	void MapAnalyzerWidget::updateInfo() {
		auto* debug = &_application.settings().simulationSettings.debug_data;
		if (!debug || !debug->selectedNode) {
			_nodeId->setText("Node: none");
			_coords->setText("Coords: -");
		} else {
			const auto* node = debug->selectedNode;
			_nodeId->setText(QString("Node: %1").arg(node->uid));
			_coords->setText(
				QString("Coords: %1, %2").arg(node->coordinates.latitude).arg(node->coordinates.longitude));
		}

		auto& connection = _application.uiSystem().get_render_data_connection();
		connection.read([this](const core::model::MapRendererData& render_data) {
			_networkOnly->setChecked(render_data.get_network_only_for_selected());
		});
	}

	void MapAnalyzerWidget::onNetworkOnlyChanged(int state) {
		const bool value = state == Qt::Checked;
		auto v_sys = _application.systems().get<visualization::VisualSystem>();
		v_sys->commands().add_command(
			visualization::UpdateRenderParamsCommand { .network_only_for_selected = value });
	}

} // namespace tjs::ui

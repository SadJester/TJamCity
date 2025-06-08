#include "ui_system/debug_ui/map_analyzer_widget.h"
#include "Application.h"
#include "data/persistent_render_data.h"

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

		QTimer* timer = new QTimer(this);
		connect(timer, &QTimer::timeout, this, &MapAnalyzerWidget::updateInfo);
		timer->start(200);
	}

	void MapAnalyzerWidget::updateInfo() {
		auto* cache = _application.stores().get_model<core::model::PersistentRenderData>();
		if (!cache || !cache->selectedNode) {
			_nodeId->setText("Node: none");
			_coords->setText("Coords: -");
			return;
		}

		const auto* node = cache->selectedNode->node;
		_nodeId->setText(QString("Node: %1").arg(node->uid));
		_coords->setText(QString("Coords: %1, %2").arg(node->coordinates.latitude).arg(node->coordinates.longitude));
	}

} // namespace tjs::ui

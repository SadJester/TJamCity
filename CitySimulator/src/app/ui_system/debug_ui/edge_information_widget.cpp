#include "ui_system/debug_ui/edge_information_widget.h"
#include "Application.h"
#include "data/map_renderer_data.h"

#include <core/data_layer/world_data.h>
#include <core/data_layer/enums.h>
#include <core/enum_flags.h>
#include <QVBoxLayout>
#include <QStringList>

namespace tjs::ui {

	static QString orientation_to_string(core::LaneOrientation o) {
		return o == core::LaneOrientation::Forward ? "Forward" : "Backward";
	}

	static QString turn_to_string(core::TurnDirection t) {
		if (t == core::TurnDirection::None) {
			return "None";
		}
		QStringList parts;
		if (has_flag(t, core::TurnDirection::Left)) {
			parts << "Left";
		}
		if (has_flag(t, core::TurnDirection::Right)) {
			parts << "Right";
		}
		if (has_flag(t, core::TurnDirection::Straight)) {
			parts << "Straight";
		}
		if (has_flag(t, core::TurnDirection::UTurn)) {
			parts << "UTurn";
		}
		if (has_flag(t, core::TurnDirection::MergeRight)) {
			parts << "MergeRight";
		}
		if (has_flag(t, core::TurnDirection::MergeLeft)) {
			parts << "MergeLeft";
		}
		return parts.join("|");
	}

	EdgeInformationWidget::EdgeInformationWidget(Application& app)
		: QWidget(nullptr)
		, _application(app) {
		QVBoxLayout* layout = new QVBoxLayout(this);
		_tree = new QTreeWidget(this);
		_tree->setHeaderHidden(true);
		layout->addWidget(_tree);

		_info = new QLabel(this);
		_info->setWordWrap(true);
		layout->addWidget(_info);

		populateTree();
		connect(_tree, &QTreeWidget::itemClicked, this, &EdgeInformationWidget::handleItemClicked);

		_application.message_dispatcher().register_handler(*this, &EdgeInformationWidget::handle_lane_selected, "EdgeInformationWidget");
		_application.message_dispatcher().register_handler(*this, &EdgeInformationWidget::handle_open_map, "EdgeInformationWidget");
	}

	EdgeInformationWidget::~EdgeInformationWidget() {
		_application.message_dispatcher().unregister_handler<events::LaneIsSelected>("EdgeInformationWidget");
		_application.message_dispatcher().unregister_handler<events::OpenMapEvent>("EdgeInformationWidget");
	}

	void EdgeInformationWidget::populateTree() {
		_tree->clear();
		_rootItem = new QTreeWidgetItem(_tree);
		_rootItem->setText(0, "Edges");
		_rootItem->setExpanded(false);

		if (_application.worldData().segments().empty()) {
			return;
		}
		auto& segment = *_application.worldData().segments().front();
		if (!segment.road_network) {
			return;
		}

		auto* render_data = _application.stores().get_model<core::model::MapRendererData>();
		if (!render_data) {
			return;
		}

		if (!render_data->selected_lane) {
			return;
		}

		for (const core::Edge& edge : segment.road_network->edges) {
			if (&edge != render_data->selected_lane->parent) {
				continue;
			}
			QTreeWidgetItem* edgeItem = new QTreeWidgetItem();
			edgeItem->setText(0, QString("Edge %1").arg(edge.get_id()));
			edgeItem->setData(0, Qt::UserRole, QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(&edge)));
			_rootItem->addChild(edgeItem);
			for (const core::Lane& lane : edge.lanes) {
				QTreeWidgetItem* laneItem = new QTreeWidgetItem();
				laneItem->setText(0, QString("Lane %1").arg(lane.get_id()));
				laneItem->setData(0, Qt::UserRole, QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(&lane)));
				edgeItem->addChild(laneItem);
			}
		}
	}

	void EdgeInformationWidget::handleItemClicked(QTreeWidgetItem* item, int) {
		auto* render = _application.stores().get_model<core::model::MapRendererData>();
		if (!render) {
			return;
		}

		if (item == _rootItem) {
			render->selected_lane = nullptr;
			_info->setText("Edges info");
			return;
		}

		if (item->parent() == _rootItem) { // edge
			const core::Edge* edge = reinterpret_cast<const core::Edge*>(item->data(0, Qt::UserRole).value<quintptr>());
			if (!edge) {
				_info->setText("-");
				return;
			}
			render->selected_lane = nullptr;
			QString text = QString("Way %1\nStart: %2\nEnd: %3\nOrientation: %4\nLanes: %5")
							   .arg(edge->way ? edge->way->uid : 0)
							   .arg(edge->start_node ? edge->start_node->uid : 0)
							   .arg(edge->end_node ? edge->end_node->uid : 0)
							   .arg(orientation_to_string(edge->orientation))
							   .arg(edge->lanes.size());
			_info->setText(text);
			return;
		}

		const core::Lane* lane = reinterpret_cast<const core::Lane*>(item->data(0, Qt::UserRole).value<quintptr>());
		if (!lane) {
			_info->setText("-");
			return;
		}
		render->selected_lane = const_cast<core::Lane*>(lane);

		QStringList outs;
		for (const auto& link : lane->outgoing_connections) {
			if (link->to) {
				outs << QString::number(link->to->get_id());
			}
		}
		QStringList ins;
		for (const auto& link : lane->incoming_connections) {
			if (link->from) {
				ins << QString::number(link->from->get_id());
			}
		}

		QString text = QString("Lane %1\nWidth: %2\nTurn: %3\nOutgoing: %4\nIncoming: %5")
						   .arg(lane->get_id())
						   .arg(lane->width)
						   .arg(turn_to_string(lane->turn))
						   .arg(outs.join(", "))
						   .arg(ins.join(", "));
		_info->setText(text);
	}

	void EdgeInformationWidget::handle_lane_selected(const events::LaneIsSelected& event) {
		populateTree();
	}

	void EdgeInformationWidget::handle_open_map(const events::OpenMapEvent& event) {
		populateTree();
	}

} // namespace tjs::ui

#pragma once

#include <core/store_models/idata_model.h>

namespace tjs::core {
	struct AgentData;
} // namespace tjs::core

namespace tjs::core::model {
	struct VehicleAnalyzeData : public IDataModel {
		core::AgentData* agent = nullptr;

		static std::type_index get_type() {
			return typeid(VehicleAnalyzeData);
		}

		void set_agent(core::AgentData* agent) {
			this->agent = agent;
		}

		void reinit() {
			agent = nullptr;
		}
	};
} // namespace tjs::core::model

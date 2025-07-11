#pragma once

#include <core/store_models/idata_model.h>

namespace tjs {
	class Application;

	class ILogicModule : public core::IStoreEntry {
	public:
		ILogicModule(Application& application)
			: _application(application) {
		}
		virtual ~ILogicModule() {}

		void reinit() override {
		}

	protected:
		Application& _application;
	};

	using LogicHandler = core::GenericStore<ILogicModule>;
} // namespace tjs

#pragma once

namespace tjs {
	class Application;

	class IUIController {
	public:
		virtual ~IUIController() {}
		virtual void run() = 0;
		virtual void update() = 0;
	};

	class UISystem {
	public:
		UISystem(Application& application)
			: _application(application) {
		}

		void initialize();
		void update();

	private:
		std::unique_ptr<IUIController> _controller;
		Application& _application;
	};
} // namespace tjs

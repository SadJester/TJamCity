#include <stdafx.h>

#include <project/project.h>

#include <Application.h>

#include <events/project_events.h>

#include <core/data_layer/world_data.h>
#include <core/data_layer/world_creator.h>

#include <core/simulation/simulation_system.h>

namespace tjs {
	bool open_map_simulation_reinit(std::string_view fileName, Application& application) {
		if (tjs::core::WorldCreator::loadOSMData(application.worldData(), fileName)) {
			tjs::core::WorldCreator::createRandomVehicles(application.worldData(), application.settings().simulationSettings);
			application.settings().general.selectedFile = fileName;

			application.simulationSystem().initialize();

			application.stores().reinit();
			application.logic_modules().reinit();

			application.message_dispatcher().handle_message(events::OpenMapEvent {}, "project");
			return true;
		}
		return false;
	}
} // namespace tjs

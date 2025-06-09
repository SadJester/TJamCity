#pragma once

#include <string>

namespace tjs {
	class Application;

	int launch(int argc, char* argv[]);
	
	bool open_map(std::string_view fileName, Application& application);
} // namespace tjs

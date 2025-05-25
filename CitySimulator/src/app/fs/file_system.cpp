#include "stdafx.h"

#include "fs/file_system.h"
#include <ranges>

namespace tjs::fs {

	std::unordered_set<std::filesystem::path> FileLocator::_createdPaths = {};

	bool FileLocator::ensureDirectoryExists(const std::filesystem::path& p) {
		const bool created = std::ranges::find(FileLocator::_createdPaths, p) != FileLocator::_createdPaths.end();
		if (!created) {
			// TODO: Refactor when it will be good error fetching mechanism
			const bool result = std::filesystem::create_directories(p);
			_createdPaths.emplace(p.string());
			return result;
		}
		return true;
	}

	std::filesystem::path FileLocator::getApplicationDir(std::string_view appName) {
#ifdef _WIN32
		std::filesystem::path p = std::getenv("APPDATA");
#elif __APPLE__
		std::filesystem::path p = std::getenv("HOME");
		p /= "Library/Application Support";
#else
		std::filesystem::path p = std::getenv("HOME");
		p /= ".config";
#endif

		p /= appName;
		ensureDirectoryExists(p);
		return p.string();
	}

	std::string FileLocator::getConfigPath(std::string_view appName, std::string_view fileName) {
		auto appDir = getApplicationDir(appName);
		return (appDir / fileName).string();
	}
} // namespace tjs::fs

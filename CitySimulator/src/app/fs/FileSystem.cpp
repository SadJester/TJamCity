#include "stdafx.h"

#include "fs/FileSystem.h"

namespace tjs::fs {
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
        // TODO: Error handling
        std::filesystem::create_directories(p);
        return p.string();
    }

    std::string FileLocator::getConfigPath(std::string_view appName, std::string_view fileName) {
        auto appDir = getApplicationDir(appName);
        return (appDir / fileName).string();
    }
}

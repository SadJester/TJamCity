#include "stdafx.h"

#include "fs/FileSystem.h"

namespace tjs::fs {
    std::string FileLocator::getConfigPath(const std::string& appName) {
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
        std::filesystem::create_directories(p);
        return (p / "settings.json").string();
    }
}

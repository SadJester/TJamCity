#pragma once

#include <nlohmann/json.hpp>
#include "fs/FileSystem.h"


namespace tjs::settings {
    using json = nlohmann::json;

    class JsonFileHandler : public fs::DiskFileHandler<json> {
    public:
        JsonFileHandler() 
            : DiskFileHandler<json>(
                [](const json& j) { return j.dump(4); },
                [](const std::string& s) { return json::parse(s); }
            ) {
        }
    };

    template<typename T>
    concept DataTypeRequirements = requires(T& data, const std::string& key) {
        { data.value(key, std::declval<decltype(data[key])>()) };
        
        { data[key] } -> std::same_as<decltype(data[key])>;
        { data[key] = std::declval<decltype(data[key])>() };
        { std::as_const(data).value(key, std::declval<decltype(data[key])>()) };
    };

    // Основной класс настроек
    template<DataTypeRequirements DataType = json>
    class UserSettingsLoader {
    public:
        using FileHandleType = std::unique_ptr<fs::FileHandler<DataType>>;
    private:
        DataType data;
        FileHandleType fileHandler;
        std::string _configPath;

    public:
        UserSettingsLoader()
            : fileHandler(std::make_unique<JsonFileHandler>())
        {}
        explicit UserSettingsLoader(FileHandleType handler)
            : fileHandler(std::move(handler)) {
        }

        UserSettingsLoader(std::string_view configPath, FileHandleType handler)
            : fileHandler(std::move(handler))
            , _configPath(configPath) {
        }

        std::string_view getCurrentPath() const {
            return _configPath;
        }

        void setConfigPath(std::string_view configPath) {
            this->_configPath = configPath;
        }

        bool load(std::string_view appName, std::string_view fileName) {
            if(_configPath.empty()) {
                _configPath = fs::FileLocator::getConfigPath(appName, fileName);
            }
            
            json temp;
            if(fileHandler->read(_configPath, temp)) {
                data = std::move(temp);
                return true;
            }
            return false;
        }

        bool save() {
            return !_configPath.empty() && fileHandler->write(_configPath, data);
        }

        template<typename T>
        T get(const std::string& key, const T& defaultValue = {}) const {
            try {
                return data.value(key, defaultValue);
            } catch (...) {
                return defaultValue;
            }
        }

        template<typename T>
        void set(const std::string& key, T&& value) {
            data[key] = value;
        }
    }; 
}
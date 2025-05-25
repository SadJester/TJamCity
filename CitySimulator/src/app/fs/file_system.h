#pragma once
#include <filesystem>
#include <fstream>
#include <functional>

namespace tjs::fs {
	template<typename DataType>
	class FileHandler {
	public:
		using Serializer = std::function<std::string(const DataType&)>;
		using Deserializer = std::function<DataType(const std::string&)>;

		FileHandler(Serializer serialize, Deserializer deserialize)
			: serialize_(serialize)
			, deserialize_(deserialize) {
		}
		virtual ~FileHandler() = default;

		virtual bool read(const std::string& path, DataType& data) = 0;
		virtual bool write(const std::string& path, const DataType& data) = 0;

	protected:
		Serializer serialize_;
		Deserializer deserialize_;
	};

	template<typename DataType>
	class DiskFileHandler : public FileHandler<DataType> {
	public:
		DiskFileHandler(typename FileHandler<DataType>::Serializer serialize,
			typename FileHandler<DataType>::Deserializer deserialize)
			: FileHandler<DataType>(serialize, deserialize) {
		}

		bool read(const std::string& path, DataType& data) override {
			try {
				std::ifstream file(path);
				std::string content((std::istreambuf_iterator<char>(file)),
					std::istreambuf_iterator<char>());
				data = this->deserialize_(content);
				return true;
			} catch (...) {
				return false;
			}
		}

		bool write(const std::string& path, const DataType& data) override {
			try {
				std::ofstream file(path);
				file << this->serialize_(data);
				return true;
			} catch (...) {
				return false;
			}
		}
	};

	class BinaryFileHandler : public DiskFileHandler<std::vector<uint8_t>> {
	public:
		BinaryFileHandler()
			: DiskFileHandler<std::vector<uint8_t>>(
				[](const auto& data) { return std::string(data.begin(), data.end()); },
				[](const std::string& s) { return std::vector<uint8_t>(s.begin(), s.end()); }) {
		}
	};

	// Локатор файлов для разных ОС
	class FileLocator {
	public:
		static std::filesystem::path getApplicationDir(std::string_view appName);
		static std::string getConfigPath(std::string_view appName, std::string_view fileName);

	private:
		static bool ensureDirectoryExists(const std::filesystem::path& p);

	private:
		static std::unordered_set<std::filesystem::path> _createdPaths;
	};
} // namespace tjs::fs

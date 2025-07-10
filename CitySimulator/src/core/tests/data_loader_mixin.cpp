#include <stdafx.h>

#include <data_loader_mixin.h>

namespace tjs::core::tests {
	std::filesystem::path DataLoaderMixin::data_file(std::string_view name) {
		return std::filesystem::path(__FILE__).parent_path() / "test_data" / name;
	}

	std::filesystem::path DataLoaderMixin::sample_file(std::string_view name) {
		auto test_folder = std::filesystem::path(__FILE__).parent_path();
		auto repo_root = test_folder.parent_path().parent_path().parent_path().parent_path();
		return repo_root / "sample_maps" / name;
	}
} // namespace tjs::core::tests

#pragma once

namespace tjs::core::tests {

	class DataLoaderMixin {
	public:
		// File from core/tests/test_data
		std::filesystem::path data_file(std::string_view name);
		// File from sample_maps/
		std::filesystem::path sample_file(std::string_view name);
	};

} // namespace tjs::core::tests

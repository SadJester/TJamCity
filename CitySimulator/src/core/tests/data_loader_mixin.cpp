#include <stdafx.h>

#include <data_loader_mixin.h>

namespace tjs::core::tests {
    std::filesystem::path DataLoaderMixin::data_file(std::string_view name) {
        return std::filesystem::path(__FILE__).parent_path() / "test_data" / name;
    }
}

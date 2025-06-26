#pragma once


namespace tjs::core::tests {

    class DataLoaderMixin {
    public:
        std::filesystem::path data_file(std::string_view name);
    };

}

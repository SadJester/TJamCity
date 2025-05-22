#pragma once

#include "core/dataLayer/data_types.h"

namespace tjs {
    namespace core {

        template <typename EntryType>
        requires std::is_pod<EntryType>::value
        using WorldEntries = std::vector<EntryType>;

        using WorldSegments = std::vector<std::unique_ptr<WorldSegment>>;


        class WorldData final {
            public:
                WorldData() = default;
                ~WorldData() = default;
                WorldData(const WorldData&) = delete;
                WorldData(WorldData&& other) = default;

                WorldEntries<Vehicle>& vehicles() {
                    return _vehicles;
                }

                WorldSegments& segments() {
                    return _segments;
                }

            private:
                WorldEntries<Vehicle> _vehicles;
                WorldSegments _segments;
        };
    }
}
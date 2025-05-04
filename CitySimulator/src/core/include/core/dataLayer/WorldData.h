#pragma once

#include "dataLayer/DataTypes.h"

namespace tjs {
    namespace core {

        template <typename EntryType>
        requires std::is_pod<EntryType>::value
        using WorldEntries = std::vector<EntryType>;

        using Roads = int; // here will be graph

        class WorldData final {
            public:
                WorldData() = default;
                ~WorldData() = default;
                WorldData(const WorldData&) = delete;
                WorldData(WorldData&& other);

                WorldEntries<Vehicle>& vehicles() {
                    return _vehicles;
                }

            private:
                WorldEntries<Vehicle> _vehicles;
        };
    }
}
#include "stdafx.h"

#include "dataLayer/WorldData.h"

namespace tjs::core
{
    WorldData::WorldData(WorldData&& other) {
        this->_vehicles = std::move(other._vehicles);
    }
} // namespace tjs::core


#pragma once

#include "core/data_layer/data_types.h"

namespace tjs {
	namespace core {
		using WorldSegments = std::vector<std::unique_ptr<WorldSegment>>;

		class WorldData final {
		public:
			WorldData() = default;
			~WorldData() = default;
			WorldData(const WorldData&) = delete;
			WorldData(WorldData&& other) = default;

			WorldSegments& segments() {
				return _segments;
			}

		private:
			WorldSegments _segments;
		};
	} // namespace core
} // namespace tjs

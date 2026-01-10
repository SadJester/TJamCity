#pragma once

#include <core/data_layer/data_types.h>

#include <common/sync //coordinated_gate.h>

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

			common::sync::coordinated_gate_holder& gates() {
				return _gates;
			}

		private:
			WorldSegments _segments;
			common::sync::coordinated_gate_holder _gates;
		};
	} // namespace core
} // namespace tjs

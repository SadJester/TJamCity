#pragma once

#include <core/enum_flags.h>

namespace tjs::core {
	ENUM_FLAG(WayTags, None, Motorway, Trunk, Primary, Secondary, Tertiary, Residential, Service);

	struct Node;

	struct WayInfo {
		uint64_t uid;
		int lanes;
		int maxSpeed;
		WayTags tags;
		std::vector<uint64_t> nodeRefs;
		std::vector<Node*> nodes;

		static std::unique_ptr<WayInfo> create(uint64_t uid, int lanes, int maxSpeed, WayTags tags) {
			auto way = std::make_unique<WayInfo>();
			way->uid = uid;
			way->lanes = lanes;
			way->maxSpeed = maxSpeed;
			way->tags = tags;
			return way;
		}
	};

} // namespace tjs::core

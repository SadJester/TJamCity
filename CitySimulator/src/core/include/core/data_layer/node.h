#pragma once

namespace tjs::core {
	ENUM_FLAG(NodeTags, char, None, TrafficLight, StopSign, Crosswalk, Way);

	struct WayInfo;
	struct Node {
		uint64_t uid;
		Coordinates coordinates;
		NodeTags tags;

		// Additional data for faster search
		std::vector<WayInfo*> ways;

		static std::unique_ptr<Node> create(uint64_t uid, const Coordinates& coordinates, NodeTags tags) {
			auto node = std::make_unique<Node>();
			node->uid = uid;
			node->coordinates = coordinates;
			node->tags = tags;
			return node;
		}

		bool hasTag(NodeTags tag) const {
			return has_flag(tags, tag);
		}
	};
	// static_assert(std::is_pod<Node>::value, "Data object expect to be POD");

	struct Junction {
		uint64_t uid;
		Node* node;
		std::vector<WayInfo*> connectedWays;
	};

} // namespace tjs::core

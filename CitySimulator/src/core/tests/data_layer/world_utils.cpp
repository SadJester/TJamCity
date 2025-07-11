#include <stdafx.h>

#include <data_layer/world_utils.h>

#include <core/data_layer/way_info.h>

namespace tjs::core::tests {
	std::unique_ptr<core::WayInfo>
		make_way(std::uint64_t uid,
			std::size_t fwd,
			std::size_t back,
			double lane_width) {
		auto w = core::WayInfo::create(uid, static_cast<int>(fwd + back),
			/*maxSpeed=*/50, WayType::Residential, WayTag::None, 0);
		w->lanesForward = static_cast<int>(fwd);
		w->lanesBackward = static_cast<int>(back);
		w->laneWidth = lane_width;

		const std::vector<TurnDirection> seq {
			TurnDirection::Straight, TurnDirection::Right,
			TurnDirection::Left, TurnDirection::UTurn,
			TurnDirection::MergeRight, TurnDirection::MergeLeft
		};
		w->forwardTurns.resize(fwd);
		for (std::size_t i = 0; i < fwd; ++i) {
			w->forwardTurns[i] = seq[i % seq.size()];
		}

		w->backwardTurns.resize(back);
		for (std::size_t i = 0; i < back; ++i) {
			w->backwardTurns[i] = seq[(i + 3) % seq.size()];
		}

		return w;
	}
} // namespace tjs::core::tests

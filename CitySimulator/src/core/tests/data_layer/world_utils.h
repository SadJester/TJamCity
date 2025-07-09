#pragma once

namespace tjs::core {
	struct WayInfo;
} // namespace tjs::core

namespace tjs::core::tests {
	std::unique_ptr<core::WayInfo> make_way(uint64_t uid, size_t fwd, size_t back, double lane_width);
} // namespace tjs::core::tests

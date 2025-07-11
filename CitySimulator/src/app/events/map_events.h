#pragma once

#include <common/message_dispatcher/Event.h>

namespace tjs::events {
	struct MapPositioningChanged : common::Event {};
	struct LaneIsSelected : common::Event {};
} // namespace tjs::events

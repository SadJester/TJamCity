#pragma once

#include <common/message_dispatcher/Event.h>

namespace tjs::core::events {
	struct VehiclesPopulated : common::Event {
		size_t generated = 0;
		size_t current = 0;
		size_t total = 0;
		size_t creation_ticks = 0;
		bool error = false;

		VehiclesPopulated(size_t gen, size_t cur, size_t tot, size_t ticks, bool err = false)
			: generated(gen)
			, current(cur)
			, total(tot)
			, creation_ticks(ticks)
			, error(err) {}
	};
} // namespace tjs::core::events

#pragma once

#include <core/utils/debugger.h>

#ifdef TRACY_ENABLED
#include <tracy/Tracy.hpp>

#define TJS_TRACY ZoneScoped
#define TJS_TRACY_NAMED(NAME) ZoneScopedN(NAME)
#define TJS_FRAME_MARK FrameMark
#else
#define TJS_TRACY
#define TJS_TRACY_NAMED(NAME)
#define TJS_FRAME_MARK
#endif

// TODO[simulation]: should be configurable
#define TJS_SIMULATION_DEBUG 1

#if TJS_SIMULATION_DEBUG
#define TJS_BREAK_IF(expr)                \
	do {                                  \
		if (expr)                         \
			tjs::debugger::debug_break(); \
	} while (0)
#else
#define TJS_BREAK_IF(expr)
#endif

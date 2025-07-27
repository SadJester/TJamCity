#pragma once

#include <csignal>

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
inline void debug_break() {
#if defined(_MSC_VER)
	__debugbreak();
#elif defined(__GNUC__) || defined(__clang__)
#if defined(__i386__) || defined(__x86_64__)
	__asm__ volatile("int3");
#else
	std::raise(SIGTRAP); // portable on POSIX
#endif
#else
	*(volatile int*)0 = 0;
#endif
}
#define TJS_BREAK_IF(expr) \
	do {                   \
		if (expr)          \
			debug_break(); \
	} while (0)
#else
inline void debug_break() {}
#define TJS_BREAK_IF(expr)
#endif

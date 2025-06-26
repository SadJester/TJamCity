#pragma once

#include <core/core_includes.h>

#include <pugixml.hpp>

#ifdef TRACY_ENABLED
#define TRACY_ENABLE
#include <tracy/Tracy.hpp>

#define TJS_TRACY ZoneScoped
#define TJS_TRACY_NAMED(NAME) ZoneScopedN(NAME)
#define TJS_FRAME_MARK FrameMark
#else
#define TJS_TRACY
#define TJS_TRACY_NAMED(NAME)
#define TJS_FRAME_MARK
#endif

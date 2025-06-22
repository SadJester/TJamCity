#pragma once

#ifdef TRACY_ENABLED
#  include <tracy/tracy/Tracy.hpp>

#  define TJS_TRACY ZoneScoped
#  define TJS_TRACY_NAMED(NAME) ZoneScopedN(NAME)
#  define TJS_FRAME_MARK FrameMark
#else
#  define TJS_TRACY
#  define TJS_TRACY_NAMED(NAME)
#  define TJS_FRAME_MARK
#endif

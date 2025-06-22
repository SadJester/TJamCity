#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <deque>

#include <thread>
#include <typeindex>
#include <algorithm>
#include <iostream>
#include <span>
#include <optional>
#include <variant>
#include <ranges>
#include <concepts>
#include <utility>

#ifdef TRACY_ENABLED
#  define TRACY_ENABLE
#  include <Tracy.hpp>

#  define TJS_TRACY ZoneScoped
#  define TJS_TRACY_NAMED(NAME) ZoneScopedN(NAME)
#  define TJS_FRAME_MARK FrameMark
#else
#  define TJS_TRACY
#  define TJS_TRACY_NAMED(NAME)
#  define TJS_FRAME_MARK
#endif

#pragma once

#include <cstdint>

#define ENUM_FLAG(EnumName, Type, ...)                                                \
	enum class EnumName : Type { __VA_ARGS__ };                                       \
	inline EnumName operator|(EnumName a, EnumName b) {                               \
		return static_cast<EnumName>(static_cast<Type>(a) | static_cast<Type>(b));    \
	}                                                                                 \
	inline EnumName operator&(EnumName a, EnumName b) {                               \
		return static_cast<EnumName>(static_cast<Type>(a) & static_cast<Type>(b));    \
	}                                                                                 \
	inline Type operator&(EnumName a, Type b) {                                       \
		return static_cast<Type>(a) & b;                                              \
	}                                                                                 \
	inline Type operator&(Type a, EnumName b) {                                       \
		return a & static_cast<Type>(b);                                              \
	}                                                                                 \
	inline bool has_flag(EnumName a, EnumName b) {                                    \
		return (static_cast<Type>(a) & static_cast<Type>(b)) == static_cast<Type>(b); \
	}

#define ENUM(EnumName, Type, ...)             \
	enum class EnumName : Type { __VA_ARGS__, \
		Count };

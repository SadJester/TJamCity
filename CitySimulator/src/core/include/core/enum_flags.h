#pragma once

#define ENUM_FLAG(EnumName, ...)                                                 \
	enum class EnumName { __VA_ARGS__,                                           \
		Count };                                                                 \
	inline EnumName operator|(EnumName a, EnumName b) {                          \
		return static_cast<EnumName>(static_cast<int>(a) | static_cast<int>(b)); \
	}                                                                            \
	inline EnumName operator&(EnumName a, EnumName b) {                          \
		return static_cast<EnumName>(static_cast<int>(a) & static_cast<int>(b)); \
	}                                                                            \
	inline bool hasFlag(EnumName a, EnumName b) {                                \
		return (static_cast<int>(a) & static_cast<int>(b)) != 0;                 \
	}

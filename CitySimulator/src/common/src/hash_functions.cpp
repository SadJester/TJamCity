#include <common/stdafx.h>

#include <common/hash_functions.h>

namespace tjs::common {
	static std::hash<std::string> hash_func;
	size_t hash_function(const std::string& str) {
		return hash_func(str);
	}
} // namespace tjs::common

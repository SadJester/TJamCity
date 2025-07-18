#pragma once

#include <common/const_string.h>

namespace tjs::common {
	namespace impl {
		constexpr size_t A = 54059; /* a prime */
		constexpr size_t B = 76963; /* another prime */
		constexpr size_t C = 86969; /* yet another prime */
	} // namespace impl

	constexpr size_t hash_str(ConstString s, size_t h = 31, size_t i = 0) {
		return i == s.size() ? h // or h % impl::C
							   :
							   hash_str(s,
								   (h * impl::A) ^ (s[i] * impl::B),
								   i + 1);
	}

	template<size_t hash_value>
	struct ConstStrHash {
		static constexpr size_t hash = hash_value;
	};

	constexpr size_t hash_function(ConstString str) {
		return common::hash_str(str);
	}

	template<class T>
	size_t hash_function(const T& val) {
		static std::hash<T> hash;
		return hash(val);
	}

	struct PairHash {
		template<typename T1, typename T2>
		std::size_t operator()(const std::pair<T1, T2>& p) const {
			auto hash1 = std::hash<T1> {}(p.first);
			auto hash2 = std::hash<T2> {}(p.second);
			return hash1 ^ (hash2 << 1);
		}
	};

} // namespace tjs::common

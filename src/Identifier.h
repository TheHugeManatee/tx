#pragma once

#include <array>
#include <numeric>
#include <stdint.h>
#include <string>
#include <functional>

#include <cassert>

#include "utils.h"

namespace /* Helpers for the Identifier class should not be interesting for anyone outside*/ {
	/// constexpr implementation for strlen
	int CONSTEXPR strlen_cexp(const char* str) {
		return *str ? 1 + strlen_cexp(str + 1) : 0;
	}

	/// accumulates a hash from an array by xor'ing all elements
	CONSTEXPR uint64_t accum_hash(const uint64_t* begin, const uint64_t* end, uint64_t start) {
		return begin == end ? (start ^ *begin) : (*begin ^ accum_hash(begin+1, end, start));
	}

	/// sets a single byte read from an array at the appropriate position of a 64-bit int
	template<uint64_t N>
	CONSTEXPR uint64_t setByte(const char(&n)[N], size_t iInd, size_t bInd) {
		return (8 * iInd + bInd + 1 < N) ? (uint64_t(n[8 * iInd + bInd]) << (bInd * 8)) : 0;
	}

	/// composes a 64-bit int from four bytes read from the appropriate indices of a
	template <size_t N>
	CONSTEXPR size_t setLong(const char (&n)[N], size_t iInd) {
		return setByte(n, iInd, 7) + setByte(n, iInd, 6) + setByte(n, iInd, 5) + setByte(n, iInd, 4) +
			setByte(n, iInd, 3) + setByte(n, iInd, 2) + setByte(n, iInd, 1) + setByte(n, iInd, 0);
	}

	/// recursive constexpr implementation to check for array equality starting at position i
	template<typename T, size_t N>
	bool CONSTEXPR array_equal(const std::array<T, N>& a, const std::array<T, N>& b, size_t i) {
		return i == N ? true : a[i] == b[i] && array_equal(a, b, i + 1);
	}

	/// recursive constexpr implementation for an equality check for arrays
	template<typename T, size_t N>
	bool CONSTEXPR array_equal(const std::array<T, N>& a, const std::array<T, N>& b) {
		return array_equal(a, b, 0);
	}

	template<typename T, size_t N>
	bool CONSTEXPR array_less(const std::array<T, N>& a, const std::array<T, N>& b, size_t i) {
		return i == N ?
			false : // if we are at the end, arrays are equal
			a[i] == b[i] ?
				array_less(a, b, i + 1) :
				a[i] < b[i];

	}

	/// recursive constexpr implementation for an array equality check
	template<typename T, size_t N>
	bool CONSTEXPR array_less(const std::array<T, N>& a, const std::array<T, N>& b) {
		return array_less(a, b, 0);
	}

}

class Identifier {
public:
	static const unsigned int NUM_WORDS = 4;
	static const unsigned int MAX_LENGTH = NUM_WORDS * sizeof(uint64_t);

#if defined(_MSC_VER) && _MSC_VER <= 1800
	CONSTEXPR Identifier(uint64_t i0, uint64_t i1, uint64_t i2, uint64_t i3) {
		id_[0] = i0;
		id_[1] = i1;
		id_[2] = i2;
		id_[3] = i3;
	};

	template <size_t N>
	CONSTEXPR Identifier(const char(&name)[N]) {
		id_[0] = setLong(name, 0);
		id_[1] = setLong(name, 1);
		id_[2] = setLong(name, 2);
		id_[3] = setLong(name, 3);
	}
#else
	CONSTEXPR Identifier(uint64_t i0, uint64_t i1, uint64_t i2, uint64_t i3) : id_{ i0, i1, i2, i3 } {};

	template <size_t N>
	CONSTEXPR Identifier(const char(&name)[N])
		: id_{ { setLong(name,0),setLong(name,1),setLong(name,2),setLong(name,3) } } // could not find a generic implementation for this
	{}
#endif

	CONSTEXPR uint64_t hash() const {
		return accum_hash(&id_[0], &id_[NUM_WORDS-1], 0);
	}

	std::string name() const {
		std::string n(std::begin(name_), std::end(name_)); // construct the string from the data
		return n.substr(0, n.find_first_of('\0')); // truncate trailing zeros (for readability, technically this might lose data)
	}

	CONSTEXPR bool operator==(const Identifier& other) const {
		return array_equal(id_, other.id_);
	}

	CONSTEXPR bool operator<(const Identifier& other) const {
		return array_less(id_, other.id_);
	}

private:
	const union {
		std::array<uint64_t, NUM_WORDS> id_;
		std::array<char, MAX_LENGTH> name_;
	};
};

STRONG_TYPEDEF(Identifier, ComponentID);
STRONG_TYPEDEF(Identifier, EntityID);

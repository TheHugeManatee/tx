#pragma once

#include <array>
#include <numeric>
#include <stdint.h>
#include <string>
#include <functional>

#include <cassert>

// constexpr string, from Scott Schurr's presentation at CppNow! 2012
class str_const {
private:
	const char* const p_;
	const std::size_t sz_;
public:
	template<std::size_t N>
	constexpr str_const(const char(&a)[N]) : // ctor
		p_(a), sz_(N - 1) {}
	constexpr char operator[](std::size_t n) const { // []
		return n < sz_ ? p_[n] :
			throw std::out_of_range("");
	}
	constexpr std::size_t size() const { return sz_; } // size()
};


namespace /* Helpers for the Identifier class should not be interesting for anyone outside*/ {
	/// constexpr implementation for strlen
	int constexpr strlen_cexp(const char* str) {
		return *str ? 1 + strlen_cexp(str + 1) : 0;
	}

	/// accumulates a hash from an array by xor'ing all elements
	constexpr uint64_t accum_hash(const uint64_t* begin, const uint64_t* end, uint64_t start) {
		return begin == end ? (start ^ *begin) : (*begin ^ accum_hash(begin+1, end, start));
	}

	/// sets a single byte read from an array at the appropriate position of a 64-bit int
	template<uint64_t N>
	constexpr uint64_t setByte(const char(&n)[N], size_t iInd, size_t bInd) {
		return (8 * iInd + bInd + 1 < N) ? (uint64_t(n[8 * iInd + bInd]) << (bInd * 8)) : 0;
	}

	/// composes a 64-bit int from four bytes read from the appropriate indices of a
	template <size_t N>
	constexpr size_t setLong(const char (&n)[N], size_t iInd) {
		return setByte(n, iInd, 7) + setByte(n, iInd, 6) + setByte(n, iInd, 5) + setByte(n, iInd, 4) +
			setByte(n, iInd, 3) + setByte(n, iInd, 2) + setByte(n, iInd, 1) + setByte(n, iInd, 0);
	}

	/// recursive constexpr implementation to check for array equality starting at position i
	template<typename T, size_t N>
	bool constexpr array_equal(const std::array<T, N>& a, const std::array<T, N>& b, size_t i) {
		return i == N ? true : a[i] == b[i] && array_equal(a, b, i + 1);
	}

	/// recursive constexpr implementation for an equality check for arrays
	template<typename T, size_t N>
	bool constexpr array_equal(const std::array<T, N>& a, const std::array<T, N>& b) {
		return array_equal(a, b, 0);
	}

}

class Identifier {
public:
	static constexpr unsigned int NUM_WORDS = 4;
	static constexpr unsigned int MAX_LENGTH = NUM_WORDS * sizeof(uint64_t);

	constexpr Identifier(uint64_t i0, uint64_t i1, uint64_t i2, uint64_t i3) : id_{ i0, i1, i2, i3 } {};

	template <size_t N>
	constexpr Identifier(const char (&name)[N]) 
		: id_{ setLong(name,0),setLong(name,1),setLong(name,2),setLong(name,3)} // could not find a generic implementation for this
	{}

	constexpr uint64_t hash() const {
		return accum_hash(&id_[0], &id_[NUM_WORDS-1], 0);
	}

	std::string name() const {
		return std::string(std::begin(name_), std::end(name_));
	}

	constexpr bool operator==(const Identifier& other) const {
		return array_equal(id_, other.id_);
	}

private:
	const union {
		std::array<uint64_t, NUM_WORDS> id_;
		std::array<char, MAX_LENGTH> name_;
	};
};

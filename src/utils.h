#pragma once

// strong typedefs taken from http://stackoverflow.com/a/28928606/2415419

#include <utility>
#include <type_traits>

#define STRONG_TYPEDEF(target_type, new_type) \
    auto name_seed_##new_type = [](){}; \
    using new_type = strong_typedef<target_type, decltype(name_seed_##new_type)>;

template <typename T, typename NameSeed,
	typename Enable = typename std::conditional<!std::is_fundamental<T>::value,
	std::true_type,
	std::false_type>::type>
	struct strong_typedef;

//specialization for non fundamental types
template <typename T, typename NameSeed>
struct strong_typedef<T, NameSeed, std::true_type> : public T
{
	template <typename... Args>
	strong_typedef(Args&&... args) : T(std::forward<Args>(args)...) {}
};

//specialization for fundamental types
template <typename T, typename NameSeed>
struct strong_typedef<T, NameSeed, std::false_type>
{
	T t;
	strong_typedef(const T t_) : t(t_) {};
	strong_typedef() {};
	strong_typedef(const strong_typedef & t_) : t(t_.t) {}
	strong_typedef & operator=(const strong_typedef & rhs) { t = rhs.t; return *this; }
	strong_typedef & operator=(const T & rhs) { t = rhs; return *this; }
	/*operator const T & () const {return t; }*/
	operator T & () { return t; }
	bool operator==(const strong_typedef & rhs) const { return t == rhs.t; }
	bool operator<(const strong_typedef & rhs) const { return t < rhs.t; }
};


// constexpr string, from Scott Schurr's presentation at CppNow! 2012
class str_const {
private:
	const char* const p_;
	const std::size_t sz_;
public:
	template<std::size_t N>
	CONSTEXPR str_const(const char(&a)[N]) : // ctor
		p_(a), sz_(N - 1) {}
	CONSTEXPR char operator[](std::size_t n) const { // []
		return n < sz_ ? p_[n] :
			throw std::out_of_range("");
	}
	CONSTEXPR std::size_t size() const { return sz_; } // size()
};

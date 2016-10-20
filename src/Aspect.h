#pragma once

#include <tuple>

template <class... C>
class Aspect {
public:

	static const size_t nCmp = sizeof...(C);
	
	static bool hasAspect(const Entity& entity) NOEXCEPT {
		return entity.hasAspect<C...>();
	}


	Aspect(C... args) : defaults_(args...) {

	}

private:
	std::tuple<C...> defaults_;
};

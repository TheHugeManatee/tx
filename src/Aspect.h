#pragma once

#include <tuple>

#include "Component.h"

template <class... C>
class Aspect {
public:

	static const size_t nCmp = sizeof...(C);
	
	Aspect(std::array < ComponentID, sizeof...(C)> ids) NOEXCEPT
		: ids_{std::move(ids)}
		//, defaults_(std::make_tuple<C...>(C()...))
	{

	}

	template<class... C>
	Aspect(std::pair<ComponentID, C>&&...  args) NOEXCEPT
		: ids_{ std::move(args.first)... } 
		//, defaults_{ std::move(args.second)... }
	{}

	bool hasAspect(const Entity& entity) const NOEXCEPT {
		size_t idx = 0;
		bool aspects[] = { entity.hasComponent<C>(ids_[idx++])... };
		for (auto flag : aspects) if (!flag) return false;
		return true;
	}

	//std::tuple<C...> defaults_;
	std::array<ComponentID, sizeof...(C)> ids_;
};

template<class... C>
Aspect<C...> make_aspect(std::pair<ComponentID, C>&&...  args)
{
	return Aspect<C...>(std::move(args)...);
}

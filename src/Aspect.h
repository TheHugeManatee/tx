#pragma once

#include "Identifier.h"

#include <tuple>

namespace tx {

    template <class... C>
    class Aspect {
    public:

        static const size_t nCmp = sizeof...(C);

        Aspect(std::array < ComponentID, sizeof...(C)> ids) NOEXCEPT
            : ids_{ std::move(ids) }
            //, defaults_(std::make_tuple<C...>(C()...))
        {

        }

        template<class... C1>
        Aspect(std::pair<ComponentID, C1>&&...  args) NOEXCEPT
            : ids_{ std::move(args.first)... }
            //, defaults_{ std::move(args.second)... }
        {}

        bool checkAspect(const Entity& entity) const NOEXCEPT {
            size_t idx = 0;
            bool aspects[] = { entity.hasComponent<C>(ids_[idx++])... };
            for (auto flag : aspects) if (!flag) return false;
            return true;
        }

        //std::tuple<C...> defaults_;
        std::array<ComponentID, sizeof...(C)> ids_;
    };

}

#include "Component.h"

template<class... C>
tx::Aspect<C...> tx::make_aspect(std::pair<tx::ComponentID, C>&&...  args)
{
    return Aspect<C...>(std::move(args)...);
}

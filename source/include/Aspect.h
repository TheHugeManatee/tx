#pragma once

#include "Entity.h"
#include "Identifier.h"

#include <tuple>
#include <type_traits>

namespace tx
{

/**
 *  Aspect class defines an "interface" that an entity can be checked against, which defines
 *  a collection of ComponentIDs and associated types.
 *
 *  // TODO: Make this class constexpr?
 */
template<class... ComponentTypes>
class Aspect
{
public:
    static const size_t nCmp = sizeof...(ComponentTypes);
    using IDs_type           = std::array<ComponentID, sizeof...(ComponentTypes)>;

    Aspect(IDs_type&& ids) noexcept : ids_{std::move(ids)} {}

    bool checkAspect(const Entity& entity) const noexcept
    {
        // TODO review this, might be unsafe cause of this funny idx++ which is unsequenced. Replace
        // with "swallow" pattern
        size_t idx              = 0;
        bool   componentFlags[] = {entity.hasComponent<ComponentTypes>(ids_[idx++])...};
        for (auto flag : componentFlags)
            if (!flag) return false;
        return true;
    }

    template<typename ComponentType>
    bool isPartOf(const ComponentID& cId) const
    {
        // TODO review this, might be unsafe cause of this funny idx++ which is unsequenced. Replace
        // with "swallow" pattern
        size_t idx              = 0;
        bool   componentFlags[] = {
            (cId == ids_[idx++] && std::is_same<ComponentTypes, ComponentType>::value)...};
        for (auto flag : componentFlags)
            if (flag) return true;
        return false;
    }

    bool isIDPartOf(const ComponentID& cId) const
    {
        for (auto id : ids_)
            if (id == cId) return true;
        return false;
    }

    const IDs_type ids_;
};

// TODO this class is not yet implemented!
template<class... ComponentTypes>
class AspectInstance : public Aspect<ComponentTypes...>
{
public:
    AspectInstance(std::pair<ComponentID, std::add_pointer_t<ComponentTypes>>... args) noexcept
        : values_{std::move(args.second)...} {};

private:
    std::tuple<std::add_pointer_t<ComponentTypes>...> values_;
};

} // namespace tx

#include "Component.h"

namespace tx
{

// template<class... ComponentTypes>
// AspectInstance<ComponentTypes...> make_aspect_instance(std::pair<ComponentID,
// std::add_pointer<ComponentTypes>::type>&&... args)
//{
//    return AspectInstance<ComponentTypes...>(std::move(args)...);
//}
} // namespace tx

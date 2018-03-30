#pragma once

#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>

#include "Identifier.h"

namespace tx
{

class ComponentBase;
template<typename WrappedClass>
class Component;
template<class... C>
class Aspect;

class Entity
{
    friend class Context;

public:
    // default constructor
    Entity() : components_(){};

    // move constructor
    Entity(Entity&& rhs) : components_(std::move(rhs.components_)) {}

    // move assignment
    Entity& operator=(Entity&& rhs)
    {
        components_ = std::move(rhs.components_);
        return *this;
    }

    // we cannot copy
    Entity(const Entity& rhs) = delete;
    Entity& operator=(const Entity& rhs) = delete;

    template<typename C>
    void setComponent(const ComponentID& id, C&& componentData) noexcept;

    template<typename C>
    bool hasComponent(const ComponentID& id) const noexcept;

    std::string toString() const;

private:
    std::unordered_map<ComponentID, std::unique_ptr<ComponentBase>> components_;

public:
    using component_iterator = decltype(components_)::iterator;
};

} // namespace tx

#include "Component.h"
#include "Identifier.h"

namespace tx
{

template<typename C>
bool Entity::hasComponent(const ComponentID& id) const noexcept
{
    auto it = components_.find(id);
    return (it != components_.end());
}

template<typename C>
void Entity::setComponent(const ComponentID& id, C&& componentData) noexcept
{
    components_.emplace(std::make_pair(id, std::make_unique<Component<C>>(componentData)));
}

std::string Entity::toString() const
{
    std::stringstream sstr;
    sstr << "Entity [";
    for (const auto& cmp : components_)
        sstr << cmp.first.name() << ": " << typeid(*cmp.second.get()).name() << "|";
    sstr << " ]";
    return sstr.str();
}

} // namespace tx

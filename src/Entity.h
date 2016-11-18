#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <set>
#include <sstream>

#include "Identifier.h"



template <class... C> class Aspect;
template <typename WrappedClass> class Component;
class ComponentBase;

class Entity {
public:
    friend class Context; // for prototyping

    void setComponent(const ComponentID& id, std::unique_ptr<ComponentBase> component) NOEXCEPT;

    template <class C>
    bool hasComponent(const Identifier& id) const NOEXCEPT;

    // default constructor
    Entity() : components_() {};

    // move constructor
    Entity(Entity&& rhs)
        : components_(std::move(rhs.components_))
    {   }

    // move assignment
    Entity& operator=(Entity&& rhs) {
        components_ = std::move(rhs.components_);
        return *this;
    }

    // explicitly deleted copy constructor and assignment
    Entity(Entity& rhs) = delete;
    Entity& operator=(Entity& rhs)  = delete;

    std::string toString() const;

private:
	std::unordered_map<ComponentID, std::unique_ptr<ComponentBase>> components_;
};

#include "Component.h"

template <class C>
bool Entity::hasComponent(const Identifier& id) const NOEXCEPT {
    auto it = components_.find(id);
    return (it != components_.end());
}

void Entity::setComponent(const ComponentID& id, std::unique_ptr<ComponentBase> component) NOEXCEPT {
    components_[id] = std::move(component);
}

std::string Entity::toString() const {
    std::stringstream sstr;
    sstr << "Entity [";
    for (const auto& cmp : components_)
        sstr << cmp.first.name() << ": " << typeid(*cmp.second.get()).name() << "|";
    sstr << " ]";
    return sstr.str();
}
#pragma once

#include "Event.h"
#include "Identifier.h"

#include <vector>
#include <unordered_map>
#include <functional>
#include <utility>
#include <type_traits>

#include <iostream>


class Entity;
class System;

class Context {
public:
    Context() {};

    ~Context() {};

    /// sets the entity with \a id, replacing a previous one if it existed
    void setEntity(const EntityID& id, Entity&& entity) {
        entities_[id] = std::move(entity);

        // TODO: think about what events exactly to emit - one componentchanged for each component of previous and new entity??
    }

    template<class S, typename... Args>
    void addSystem(Args... args);

    // templated function that takes an array of component IDs and a a function
    // will call the function for all entities that have the matching interface
    template<typename ArrayN, typename Fn>
    void each(ArrayN cIds, Fn fn);

    void each(const std::function<void(const EntityID&, Entity&)> fn);

    /**
     *	Returns an entity with the specified eId. If none exists, a new one will be created.
     */
    Entity& getEntity(const EntityID& eId) noexcept {
        return entities_[eId];
    }

    /**
    *	Returns an entity with the specified eId. If none exists, a new one will be created.
    */
    const Entity& getEntity(const EntityID& eId) const noexcept {
        return entities_[eId];
    }

    /**
     *	Returns the component of an entity. If it either entity or component do not exist, creates one using the component's
     *	default constructor.
     */
    template <typename C>
    C& getComponent(const EntityID& eId, const ComponentID& cId) const
    {
        std::string name = typeid(C).name(); // to get the type from the debugger when we hit the assertion
        assert(dynamic_cast<C*>(entities_[eId].components_[cId].get()));

        return static_cast<C&>(*entities_[eId].components_[cId]);
    }

    template <typename C, typename... Args>
    void setComponent(const EntityID& eId, const ComponentID& cId, Args... args)
    {
        entities_[eId].components_.insert(std::make_pair(cId, std::make_unique<C>(std::forward<Args>(args)...)));
    }

    template<typename C>
    void setComponent(const EntityID& eId, const ComponentID& cId, std::unique_ptr<C> component) {
        entities_[eId].components_.insert(std::make_pair(cId, std::move(component)));
    }

    /**
     *	Calls update() on all registered systems.
     */
    void updateSystems();

    /**
     *	Convenience function to extract all components for an entity and pass them to the given functional. Will generate
     *	COMPONENTCHANGED events for all components that are not accessed as refs to const.
     */
    template<typename... C, typename Fn, size_t N, size_t... CIndices, typename... FuncArgs>
    void callFuncWithComponents(Fn fn, const EntityID& eId, const std::array<ComponentID, N>& cIds, std::index_sequence<CIndices...>, FuncArgs... funcArgs) {

        fn(funcArgs..., getComponent<typename std::remove_reference<C>::type>(eId, cIds[CIndices])...);

        using swallow = int[]; // guarantees left to right order
        (void)swallow { // emits a changed event for every component that is not const qualified in the function argument list
            0, (std::is_const<typename std::remove_reference<C>::type>::value ? 0 : (void(emitEvent(Event(Event::COMPONENTCHANGED, eId, cIds[CIndices]))), 0))...
        };
    }

    void emitEvent(const Event& event) {
        std::cout << "\t\t" << "Event emitted for " << event.eId << " and " << event.cId << std::endl;
        for (auto& s : systems_) {
            if (s->isInterested(*this, event.eId)) {
                s->onEvent(event);
            }
        }
    }

    template<typename Func>
    void runSequential(Func func) {
        while (func()) {
            updateSystems();
        }
    }

protected:
    mutable std::unordered_map<EntityID, Entity> entities_; // mutable so we can still get const refs out from a const Context
    std::vector<std::unique_ptr<System>> systems_;

    /// helper functions for variadic implementation

    template <typename ArrayN, typename Fn>
    struct each_variadic_impl
        : public each_variadic_impl<ArrayN, decltype(&Fn::operator())>
    {};

    // partial specialization on the lambda function
    template <size_t N, typename ClassType, typename FirstArgType, typename ReturnType, typename... C>
    struct each_variadic_impl<std::array<ComponentID, N>, ReturnType(ClassType::*) (FirstArgType, C...) const>
    {
        template <typename FFn>
        static void impl(Context& c, std::array<ComponentID, N> cIds, FFn fn) {

            static_assert(sizeof...(C) == N, "Number of Component IDs does not match functor signature!");
            // check the functor signature
            static_assert(all_true<std::is_reference<C>::value...>::value, "Components can only be accessed through references!");
            static_assert(all_true<std::is_base_of<ComponentBase, typename std::remove_reference<C>::type>::value...>::value, "Functor signature must only contain Components (derived from ComponentBase)!");

            const auto aspect = Aspect<C...>(cIds);

            //for (auto& e : c.entities_) {
            //    if (aspect.checkAspect(e.second))
            //        c.callFuncWithComponents<C...>(fn, e.first, cIds, std::make_index_sequence<N>{}, e.first);
            //}
        };
    };
};


#include "Entity.h"
#include "System.h"

template<class S, typename... Args>
void Context::addSystem(Args... args)
{
    // TODO check if already in there
    systems_.emplace_back(std::make_unique<S>(std::forward<Args>(args)...));
    systems_.back()->init(*this);
}


void Context::each(const std::function<void(const EntityID&, Entity&)> fn) {
    //for (auto& e : entities_) {
    //    fn(e.first, e.second);
    //}
}

template<typename ArrayN, typename Fn>
void Context::each(ArrayN cIds, Fn fn) {
    each_variadic_impl<ArrayN, Fn>::impl(*this, cIds, fn);
}

void Context::updateSystems() {
    for (auto& s : systems_) {
        if (!s->isValid()) {
            if (s->update(*this))
                s->setValid();
        }
    }
}

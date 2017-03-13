#pragma once

#include "Event.h"
#include "Identifier.h"

#include <vector>
#include <unordered_map>
#include <functional>
#include <utility>
#include <type_traits>
#include <mutex>
#include <iostream>


class Entity;
class System;


class Context {
public:
    Context() {};

    ~Context() {};

    /**
     *  Create a new system. The supplied arguments are forwarded to the system's constructor.
     */
    template<class S, typename... Args>
    void emplaceSystem(Args... args);

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
     *  Sets/replaces an entity
     */
    void setEntity(const EntityID& eId, Entity&& entity) noexcept {
        entities_[eId] = std::move(entity);
    }

    template <typename C, typename... Args>
    void emplaceComponent(const EntityID& eId, const ComponentID& cId, Args... args)
    {
        entities_[eId].components_.insert(std::make_pair(cId, std::make_unique<Component<C>>(std::forward<Args>(args)...)));
    }

    template<typename C>
    void setComponent(const EntityID& eId, const ComponentID& cId, std::unique_ptr<Component<C>> component) {
        entities_[eId].components_.insert(std::make_pair(cId, std::move(component)));
    }

    template<typename... ComponentTypes>
    void setComponent(const EntityID& eId, const std::pair<ComponentID, ComponentTypes>&... cIDValues) {
        using swallow = int[]; // guarantees left to right order
        (void)swallow { // call setComponent for each pair
            0, (setComponent(eId, cIDValues.first, cIDValues.second))...
        };
    }


    /**
    *  Returns the component of an entity. If it either entity or component do not exist,
    *  an instance is created using default constructor.
    */
    template <typename C>
    bool getComponent(const EntityID& eId, const ComponentID& cId, C& componentData) const
    {
        const auto& e = entities_[eId];
        const auto& it = e.components_.find(cId);
        if (it == e.components_.end())
            return false;

        auto& cmp = it->second;

        txAssert(dynamic_cast<Component<C>*>(cmp.get()) != nullptr, "Type mismatch! Requested component type "
            << typeid(Component<C>).name() << " does not match with stored component type "
            << typeid(*cmp).name() << "!");

        componentData = *(*static_cast<Component<C>*>(cmp.get())); // dereference the pointer, then call operator* on the Component<C> instance
        return true;
    }

    /**
     *	Calls update() on all registered systems.
     */
    void updateSystems();


    /**
     *  Puts an event onto the event bus to be consumed by the systems
     */
    void emitEvent(const Event& event) {
        std::cout << "\t\t" << "Event emitted for " << event.eId << " and " << event.cId << std::endl;
        for (auto& s : systems_) {
            if (s->isInterested(*this, event.eId)) {
                s->pushEvent(event);
            }
        }
    }

    /**
     *  Runs the update systems loop until func returns false
     */
    template<typename Func>
    void runSequential(Func func) {
        while (func()) {
            updateSystems();
        }
    }

private:
    mutable std::unordered_map<EntityID, Entity> entities_; // mutable so we can still get const refs out from a const Context
    std::vector<std::unique_ptr<System>> systems_;

    /**
    *  Returns the component of an entity. If it either entity or component do not exist,
    *  an instance is created using default constructor.
    */
    template <typename ComponentType>
    ComponentType& getComponent(Entity& e, const ComponentID& cId) const
    {
        // The type of the internal component used for storage does not have a const qualifier, but C might have
        using Component_t = Component<typename std::remove_const<ComponentType>::type>;

        txAssert(e.components_.find(cId) != e.components_.end(), "Component " << cId << " does not exist!");

        auto& cmp = e.components_[cId];

        txAssert(dynamic_cast<Component_t*>(cmp.get()) != nullptr, "Type mismatch! Requested component type "
            << typeid(Component_t).name() << " does not match with stored component type "
            << typeid(*cmp).name() << "!");
        txAssert(cmp.get() != nullptr, "Component pointer is null!");

        return *(*static_cast<Component_t*>(cmp.get())); // dereference the pointer, then call operator* on the Component<C> instance
    }


    /**
    *	Convenience function to extract all components for an entity and pass them to the given functional. Will generate
    *	COMPONENTCHANGED events for all components that are not accessed as refs to const.
    */
    template<typename... C, typename Fn, size_t N, size_t... CIndices, typename... FuncArgs>
    void callFuncWithComponents(Fn fn, const EntityID& eId, const std::array<ComponentID, N>& cIds, std::index_sequence<CIndices...>, FuncArgs... funcArgs) {

        fn(funcArgs..., getComponent<typename std::remove_reference<C>::type>(entities_[eId], cIds[CIndices])...);

        using swallow = int[]; // guarantees left to right order
        (void)swallow { // emits a changed event for every component that is not const qualified in the function argument list
            0, (std::is_const<typename std::remove_reference<C>::type>::value ? 0 : (void(emitEvent(Event(Event::COMPONENTCHANGED, eId, cIds[CIndices]))), 0))...
        };
    }

    /// helper functions for variadic implementation

    template <typename ArrayN, typename Fn>
    struct each_variadic_impl
        : public each_variadic_impl<ArrayN, decltype(&Fn::operator())>
    {};

    // partial specialization on the lambda function
    template <size_t N, typename ClassType, typename FirstArgType, typename ReturnType, typename... ComponentArgs>
    struct each_variadic_impl<std::array<ComponentID, N>, ReturnType(ClassType::*) (FirstArgType, ComponentArgs...) const>
    {
        template <typename FFn>
        static void impl(Context& c, std::array<ComponentID, N> cIds, FFn fn) {
            // Check the number of components and IDs
            static_assert(sizeof...(ComponentArgs) == N, "Number of Component IDs does not match functor signature!");
            // check the functor signature
            static_assert(all_true<std::is_reference<ComponentArgs>::value...>::value, "Components can only be accessed through references!");
            //static_assert(all_true<std::is_base_of<ComponentBase, typename std::remove_reference<C>::type>::value...>::value, "Functor signature must only contain Components (derived from ComponentBase)!");

            const auto aspect = Aspect<ComponentArgs...>(cIds);

            for (auto& e : c.entities_) {
                if (aspect.checkAspect(e.second))
                    c.callFuncWithComponents<ComponentArgs...>(fn, e.first, cIds, std::make_index_sequence<N>{}, e.first);
            }
        };
    };
};


#include "Entity.h"
#include "System.h"

template<class S, typename... Args>
void Context::emplaceSystem(Args... args)
{
    // TODO check if already in there
    systems_.emplace_back(std::make_unique<S>(std::forward<Args>(args)...));
    systems_.back()->init(*this);
}


void Context::each(const std::function<void(const EntityID&, Entity&)> fn) {
    for (auto& e : entities_) {
        fn(e.first, e.second);
    }
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

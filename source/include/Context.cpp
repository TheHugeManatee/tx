#include "Context.h"

#include "Entity.h"
#include "System.h"

namespace tx {

    template<class S, typename... Args>
    void Context::emplaceSystem(Args... args)
    {
        // TODO check if already in there
        systems_.emplace_back(std::make_unique<S>(std::forward<Args>(args)...));
        systems_.back()->init(*this);
    }


    TaskFuture<size_t> Context::each(const std::function<void(const EntityID&, Entity&)>&& fn) {
        std::promise<size_t> pr;
        // TODO acquire lock over all entities
        size_t n = 0;
        for (auto& e : entities_) {
            fn(e.first, e.second);
            ++n;
        }
        pr.set_value(n);
        return pr.get_future();
    }

    template<typename ArrayN, typename Fn>
    TaskFuture<size_t> Context::each(ArrayN cIds, Fn fn) {
        return each_variadic_impl<ArrayN, Fn>::impl(*this, cIds, fn);
    }

    template<typename ArrayN, typename Fn>
    TaskFuture<size_t> Context::each(ArrayN cIds, Fn fn) const {
        return each_variadic_impl<ArrayN, Fn>::impl_const(*this, cIds, fn);
    }

    template <typename Fn>
    TaskFuture<typename function_traits<Fn>::result_type> Context::exec(Fn&& fn) {
        static_assert(function_traits<Fn>::arity == 1, "Currently, exec only supports functors with single arguments. Maybe use std::bind()?");
        static_assert(std::is_same_v<typename function_traits<Fn>::template arg<0>::type, Context::ReadOnlyProxy&> ||
                    std::is_same_v<typename function_traits<Fn>::template arg<0>::type, Context::ModifyingProxy&>,
                    "Currently only Context::ReadOnlyProxy& and Context::ModifyingProxy& are supported as arguments to the functor.");

        return exec_variadic_impl<Fn>::impl(*this, fn);
    };

    void Context::updateSystems() {
        for (auto& s : systems_) {
            if (!s->isValid()) {
                if (s->update(*this)) {
                    s->setValid();
                }
                emitEvent(Event(Event::SYSTEMUPDATED, s->getID()));
            }
        }
    }

    void Context::emitEvent(const Event& event) {
        if (event.type == Event::SYSTEMUPDATED) {
            std::cout << "\t\t" << "Event emitted for System " << event.sId << std::endl;
            for (auto& s : systems_) {
                if (s->isInterested(event.sId)) {
                    s->pushEvent(event);
                }
            }
        }
        else {
            std::cout << "\t\t" << "Event emitted for Component " << event.cId << " of Entity " << event.eId << std::endl;
            for (auto& s : systems_) {
                if (s->isInterested(*this, event.eId, event.cId)) {
                    s->pushEvent(event);
                }
            }
        }
    }

    tx::Entity& Context::getEntity(const EntityID& eId) noexcept
    {
        return entities_[eId];
    }

    const tx::Entity& Context::getEntity(const EntityID& eId) const noexcept
    {
        return entities_[eId];
    }

    void Context::setEntity(const EntityID& eId, Entity&& entity) noexcept
    {
        entities_[eId] = std::move(entity);
    }
    
    template <typename C, typename... Args>
    void tx::Context::emplaceComponent(const EntityID& eId, const ComponentID& cId, Args... args)
    {
        entities_[eId].components_.insert(std::make_pair(cId, std::make_unique<Component<C>>(std::forward<Args>(args)...)));
    }

    template<typename C>
    void tx::Context::setComponent(const EntityID& eId, const ComponentID& cId, std::unique_ptr<Component<C>> component)
    {
        entities_[eId].components_.insert(std::make_pair(cId, std::move(component)));
    }


    template<typename... ComponentTypes>
    void tx::Context::setComponent(const EntityID& eId, const std::pair<ComponentID, ComponentTypes>&... cIDValues)
    {
        using swallow = int[]; // guarantees left to right order
        (void)swallow { // call setComponent for each pair
            0, (setComponent(eId, cIDValues.first, cIDValues.second))...
        };
    }


    template <typename C>
    bool tx::Context::getComponent(const EntityID& eId, const ComponentID& cId, C& componentData) const
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


    template <typename ComponentType>
    ComponentType& tx::Context::getComponent(Entity& e, const ComponentID& cId) const
    {
        // The type of the internal component used for storage does not have a const qualifier, but C might have
        using Component_t = Component<typename std::remove_const<ComponentType>::type>;

        txAssert(e.components_.find(cId) != e.components_.end(), "Component " << cId << " does not exist!");

        auto& cmp = e.components_[cId];

        txAssert(dynamic_cast<Component_t*>(cmp.get()) != nullptr, "Type mismatch! Requested component type "
            << typeid(Component_t).name() << " does not match with stored component type "
            << typeid(*cmp).name() << "!");
        txAssert(cmp.get() != nullptr, "Component pointer is null! This should not happen.. :(");

        return *(*static_cast<Component_t*>(cmp.get())); // dereference the pointer, then call operator* on the Component<C> instance
    }


    template<typename... C, typename Fn, size_t N, size_t... CIndices, typename... FuncArgs>
    void tx::Context::callFuncWithComponents(Fn fn, const EntityID& eId, const std::array<ComponentID, N>& cIds, std::index_sequence<CIndices...>, FuncArgs... funcArgs)
    {
        fn(funcArgs..., getComponent<typename std::remove_reference<C>::type>(entities_[eId], cIds[CIndices])...);

        using swallow = int[]; // guarantees left to right order
        (void)swallow { // emits a changed event for every component that is not const qualified in the function argument list
            0, (std::is_const<typename std::remove_reference<C>::type>::value ? 0 : (void(emitEvent(Event(Event::COMPONENTCHANGED, eId, cIds[CIndices]))), 0))...
        };
    }


} // namespace tx


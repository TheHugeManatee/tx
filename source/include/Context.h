#pragma once

#include "Event.h"
#include "Identifier.h"
#include "ThreadPool.h"

#include <functional>
#include <iostream>
#include <list>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace tx {

    class Entity;
    template <typename Derived> class System;
    class SystemBase;

    /**
     *  The context is the central storage/exchange object. It handles 
     */
    class Context {
    public:
        /**
         *  Exposes a the read-only part of the context API. Handling an instance of this 
         *  proxy guarantees threadsafe access.
         */
        class ReadOnlyProxy {
        public:
            // Should only be instantiated by the parent context
            ReadOnlyProxy(Context& parent)
                : parent_(parent) {};
            virtual ~ReadOnlyProxy() {};

            /**
             *  Returns an entity with the specified eId. If none exists, a new one will be created.
             */
            const Entity& getEntity(const EntityID& eId) const noexcept {
                return parent_.entities_[eId];
            }

            /**
            *  Returns the component of an entity. If either entity or component do not exist,
            *  an instance is created using default constructor.
            */
            template <typename C>
            bool getComponent(const EntityID& eId, const ComponentID& cId, C& componentData) const {
                return parent_.getComponent(eId, cId, componentData);
            }

        protected:
            Context& parent_;
        }; // class ReadOnlyProxy

        /**
         *  Exposes read-write access to the context. Obtaining non-const access to
         *  components will generate COMPONENTCHANGED events, regardless of whether the component
         *  was actually modified.
         *  Events will be cached and emitted once this proxy object is destroyed.
         *  \note: Currently, multiple getComponent() calls for the same component will also result
         *      in multiple events.
         */
        class ModifyingProxy {
        public:
            // Should only be instantiated by the parent context
            ModifyingProxy(Context& parent)
                : parent_(parent) {};
            virtual ~ModifyingProxy() {};

            /**
            *  Returns an entity with the specified eId. If none exists, a new one will be created.
            */
            const Entity& getEntity(const EntityID& eId) const noexcept {
                return parent_.entities_[eId];
            }

            /**
            *  Returns the component of an entity. If either entity or component do not exist,
            *  an instance is created using default constructor.
            */
            template <typename C>
            bool getComponent(const EntityID& eId, const ComponentID& cId, C& componentData) const {
                return parent_.getComponent(eId, cId, componentData);
            }


            /**
            *  Sets/replaces an entity
            *  // TODO this could probably use some optimization for entities with many components.
            */
            void setEntity(const EntityID& eId, Entity&& entity) noexcept {
                Entity& eBefore = parent_.entities_[eId];
                // For all components that the old entity had, emit either REMOVED or CHANGED event
                for (auto& c : eBefore.components_) {
                    if (entity.components_.find(c.first) == entity.components_.end())
                        eventList_.emplace_back(Event::COMPONENTREMOVED, eId, c.first);
                    else
                        eventList_.emplace_back(Event::COMPONENTCHANGED, eId, c.first);
                }
                // For all new components, emit an ADDED event
                for (auto& c : entity.components_) {
                    if (eBefore.components_.find(c.first) == eBefore.components_.end())
                        eventList_.emplace_back(Event::COMPONENTADDED, eId, c.first);
                }

                parent_.entities_[eId] = std::move(entity);
            }

            /**
            *  Returns the component of an entity. If either entity or component do not exist,
            *  an instance is created using default constructor.
            */
            template <typename C>
            C* getComponentWritable(const EntityID& eId, const ComponentID& cId) {
                auto& e = parent_.entities_[eId];
                if (e.components_.find(cId) == e.components_.end()) {
                    return &parent_.getComponent<C>(e, cId);
                    eventList_.emplace_back(Event::COMPONENTCHANGED, eId, cId);
                }
                else
                    return nullptr;
            }

            template <typename C, typename... Args>
            void emplaceComponent(const EntityID& eId, const ComponentID& cId, Args... args)
            {
                auto& e = parent_.entities_[eId];
                if(e.components_.find(cId) == e.components_.end())
                    eventList_.emplace_back(Event::COMPONENTADDED, eId, cId);
                else
                    eventList_.emplace_back(Event::COMPONENTCHANGED, eId, cId);

                parent_.emplaceComponent<C>(eId, cId, std::forward<Args>(args)...);
            }


        protected:
            Context& parent_;
            std::list<Event> eventList_;
        }; // class ModifyingProxy

    public:
        Context() = default;
        ~Context() = default;
        // is only default-constructible (at the moment)
        Context(const Context &) = delete;
        Context(Context &&) = delete;
        Context& operator=(const Context &) = delete;
        Context& operator=(Context &&) = delete;

        /**
         *  Instantiate a new system. The supplied arguments are forwarded to the system's constructor.
         */
        template<class S, typename... Args>
        void emplaceSystem(Args... args);

        template<typename ArrayN, typename Fn>
        TaskFuture<size_t> each(ArrayN cIds, Fn fn);

        template<typename ArrayN, typename Fn>
        TaskFuture<size_t> each(ArrayN cIds, Fn fn) const;

        TaskFuture<size_t> each(const std::function<void(const EntityID&, Entity&)>&& fn);

        /**
         *  Executes a functional on the context. The parameters of the functional
         *  provided decide on which information is available inside that functional. Guarantees
         *  threadsafe access by supplying either ReadOnlyProxy or a ModifyingProxy.
         *
         *  The functional is expected to take either a Context::ReadOnlyProxy& or a Context::ModifyingProxy&,
         *  currently no other overloads exist.
         *
         *  \return a TaskFuture returning the result of the functional. 
         *  \note If you don't capture the returned future, this will block until the functional has
         *          been scheduled and excuted by the context, essentially turning this into a
         *          synchronous operation on the calling thread.
         */
        template <typename Fn>
        TaskFuture<typename function_traits<Fn>::result_type>exec(Fn&& fn);

        /**
         *	Calls update() on all registered systems.
         */
        void updateSystems();


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
        std::vector<std::unique_ptr<SystemBase>> systems_;

        /**
        *  [Threadsafe] Puts an event onto the event bus to be consumed by the systems.
        *
        *  Enqueues an event in all system queues that are interested.
        */
        void emitEvent(const Event& event);

        /**
        *	Returns an entity with the specified eId. If none exists, a new one will be created.
        */
        Entity& getEntity(const EntityID& eId) noexcept;

        /**
        *	Returns an entity with the specified eId. If none exists, a new one will be created.
        */
        const Entity& getEntity(const EntityID& eId) const noexcept;

        /**
        *  Sets/replaces an entity
        */
        void setEntity(const EntityID& eId, Entity&& entity) noexcept;

        template <typename C, typename... Args>
        void emplaceComponent(const EntityID& eId, const ComponentID& cId, Args... args);

        template<typename C>
        void setComponent(const EntityID& eId, const ComponentID& cId, std::unique_ptr<Component<C>> component);

        template<typename... ComponentTypes>
        void setComponent(const EntityID& eId, const std::pair<ComponentID, ComponentTypes>&... cIDValues);


        /**
        *  Returns the component of an entity. If either entity or component do not exist,
        *  an instance is created using default constructor.
        */
        template <typename C>
        bool getComponent(const EntityID& eId, const ComponentID& cId, C& componentData) const;

        /**
        *  Returns the component of an entity. If it either entity or component do not exist,
        *  an instance is created using default constructor.
        */
        template <typename ComponentType>
        ComponentType& getComponent(Entity& e, const ComponentID& cId) const;


        /**
        *	Convenience function to extract all components for an entity and pass them to the given functional. Will generate
        *	COMPONENTCHANGED events for all components that are not accessed as refs to const.
        */
        template<typename... C, typename Fn, size_t N, size_t... CIndices, typename... FuncArgs>
        void callFuncWithComponents(Fn fn, const EntityID& eId, const std::array<ComponentID, N>& cIds, std::index_sequence<CIndices...>, FuncArgs... funcArgs);

        /// helper functions and structs for variadic implementation

        template <typename ArrayN, typename Fn>
        struct each_variadic_impl
            : public each_variadic_impl<ArrayN, decltype(&Fn::operator())>
        {};

        // partial specialization of each() w.r.t. the lambda function as a struct
        template <size_t N, typename ClassType, typename FirstArgType, typename ReturnType, typename... ComponentArgs>
        struct each_variadic_impl<std::array<ComponentID, N>, ReturnType(ClassType::*) (FirstArgType, ComponentArgs...) const>
        {
            /**
             *  Implements each() with an array of known component types and a functional type FFn
             */
            template <typename FFn>
            static tx::TaskFuture<size_t> impl(Context& c, std::array<ComponentID, N> cIds, FFn fn) {
                std::promise<size_t> pr;

                // Check the number of components and IDs
                static_assert(sizeof...(ComponentArgs) == N, "Number of Component IDs does not match functor signature!");
                // check the functor signature
                static_assert(all_true<std::is_reference<ComponentArgs>::value...>::value, "Components can only be accessed through references!");
                //static_assert(all_true<std::is_base_of<ComponentBase, typename std::remove_reference<C>::type>::value...>::value, "Functor signature must only contain Components (derived from ComponentBase)!");

                const auto aspect = Aspect<ComponentArgs...>(cIds);

                size_t n = 0;
                for (auto& e : c.entities_) {
                    if (aspect.checkAspect(e.second)) {
                        c.callFuncWithComponents<ComponentArgs...>(fn, e.first, cIds, std::make_index_sequence<N>{}, e.first);
                        ++n;
                    }
                }

                pr.set_value(n);
                return pr.get_future();
            };

            /**
            *  Implements each() with an array of known component types and a functional type FFn, const version
            */
            template <typename FFn>
            static tx::TaskFuture<size_t> impl_const(const Context& c, std::array<ComponentID, N> cIds, FFn fn) {
                std::promise<size_t> pr;

                // Check the number of components and IDs
                static_assert(sizeof...(ComponentArgs) == N, "Number of Component IDs does not match functor signature!");
                // check the functor signature
                static_assert(all_true<std::is_reference<ComponentArgs>::value...>::value, "Components can only be accessed through references!");
                static_assert(all_true<std::is_const<ComponentArgs>::value...>::value, "Const version can only access const references!");
                //static_assert(all_true<std::is_base_of<ComponentBase, typename std::remove_reference<C>::type>::value...>::value, "Functor signature must only contain Components (derived from ComponentBase)!");

                const auto aspect = Aspect<ComponentArgs...>(cIds);

                size_t n = 0;
                for (auto& e : c.entities_) {
                    if (aspect.checkAspect(e.second)) {
                        c.callFuncWithComponents<ComponentArgs...>(fn, e.first, cIds, std::make_index_sequence<N>{}, e.first);
                        ++n;
                    }
                }

                pr.set_value(n);
                return pr.get_future();
            };
        };

        template <typename Fn>
        struct exec_variadic_impl
            : public exec_variadic_impl<decltype(&Fn::operator())>
        {};

        // partial specialization of exec() for void-returning functionals
        template <typename ClassType, typename FirstArgType>
        struct exec_variadic_impl<void(ClassType::*) (FirstArgType&) const>
        {
            template <typename Fn>
            static TaskFuture<void> impl(Context& c, const Fn& fn) {
                // TODO: acquire read-only lock on the whole context
                std::promise<void> pr;
                FirstArgType proxy(c);
                fn(proxy);
                pr.set_value();
                return pr.get_future();
            }
        };
        // partial specialization of exec() for other functionals
        template <typename ClassType, typename FirstArgType, typename ReturnType>
        struct exec_variadic_impl<ReturnType(ClassType::*) (FirstArgType&) const>
        {
            template <typename Fn>
            static TaskFuture<ReturnType> impl(Context& c, const Fn& fn) {
                // TODO: acquire read-only lock on the whole context
                std::promise<ReturnType> pr;
                FirstArgType proxy(c);
                pr.set_value(fn(proxy));
                return pr.get_future();
            }
        };
    };

} // namespace tx

#include "Context.cpp"

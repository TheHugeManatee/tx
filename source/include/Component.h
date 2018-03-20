#pragma once

#include <typeinfo>

namespace tx {

    // polymorphic base class for all components
    class ComponentBase {
    public:
        virtual ~ComponentBase() {};

        size_t hash() const noexcept {
            return typeid(*this).hash_code();
        }
    };

    /**
     *  Templated class to easily define user-specified component types
     */
    template <typename WrappedClass>
    class Component : public ComponentBase {
    public:
        using IDValuePair = std::pair<ComponentID, WrappedClass>;

        // default constructor
        Component()
            : value()
        {
            static_assert(std::is_move_constructible<WrappedClass>::value || std::is_copy_constructible<WrappedClass>::value, "Component Data Class must be move or copy constructible!");
        };

        // variadic constructor with at least one argument
        template <typename Arg1, typename... Args>
        Component(Arg1 arg1, Args... args)
            : value(std::forward<Arg1>(arg1), std::forward<Args>(args)...)
        {};

        Component(WrappedClass&& wrapped)
            : value(wrapped)
        {}

        virtual ~Component() {};

        Component& operator=(Component&& cmp) = delete;
        Component(Component& other) = delete;
        Component(Component&& other) = delete;

        WrappedClass * operator->() {
            return &value;
        }

        const WrappedClass * operator->() const {
            return &value;
        }

        WrappedClass & operator*() {
            return value;
        }

        const WrappedClass & operator*() const {
            return value;
        }

    private:
        WrappedClass value;
    };

    template <typename WrappedClass>
    class Cmp : public ComponentID
    {/**/
    };

} // namespace tx

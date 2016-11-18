#pragma once

#include <typeinfo>

// polymorphic base class for all components
class ComponentBase {
public:
	virtual ~ComponentBase() {}; 

	size_t hash() const NOEXCEPT {
		return typeid(*this).hash_code();
	}
};

template <typename WrappedClass>
class Component : public ComponentBase {
public:
	// default constructor
	Component()
		: value()
	{};

	// variadic constructor with at least one argument
	template <typename Arg1, typename... Args>
	Component(Arg1 arg1, Args... args)
		: value(std::forward<Arg1>(arg1), std::forward<Args>(args)...)
	{};

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

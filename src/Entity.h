#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <set>
#include <sstream>

#include "Identifier.h"
#include "Component.h"

template <class... C> class Aspect;

namespace {
	/// Implementation class needed for variadic template pattern
	template <class... C>
	struct ensureComponents_Impl;
}

class Entity {
public:
	friend class Context; // for prototyping

	void setComponent(const ComponentID& id, std::unique_ptr<Component> component) NOEXCEPT {
		components_[id] = std::move(component);
	}

	template <class C>
	bool hasComponent(const Identifier& id) const NOEXCEPT {
		auto it = components_.find(id);
		return (it != components_.end());
	}

	//template <class C>
	//C& getComponent(const Identifier& id) const NOEXCEPT {
	//	auto it = components_.find(id);
	//	if (it != components_.end()) {
	//		return *std::dynamic_pointer_cast<C>(it->second);
	//	}
	//	return std::shared_ptr<C>();
	//}

	//template <class... C>
	//bool hasAspect() const NOEXCEPT {
	//	// a set of hash codes that we will clear
	//	std::set<size_t> hashCodes{ (typeid(C).hash_code())... };

	//	for (auto& cmp : components_) {
	//		hashCodes.erase(cmp.first);
	//		if (hashCodes.empty())	return true;
	//	}
	//	return false;
	//}

	//template <template <class...> class A, class... C>
	//bool hasAspect() const NOEXCEPT {
	//	hasAspect<C...>();
	//}

	//template <class... C>
	//void ensureComponents(); // implemented below

	std::string stringify() const {
		std::stringstream sstr;
		sstr << "Entity [";
		for (const auto& cmp : components_)
			sstr << cmp.first.name() << ": " << typeid(*cmp.second.get()).name() << "|";
		sstr << " ]";
		return sstr.str();
	}

private:
	std::unordered_map<ComponentID, std::unique_ptr<Component>> components_;
};

namespace {
	// base implementation template
	template <class... C>
	struct ensureComponents_Impl;

	// general recursion specialization
	template <class C, class... Ctail>
	struct ensureComponents_Impl<C, Ctail...> {
		static void ensure(Entity& e) {
			if (!e.hasComponent<C>())
				e.setComponent(std::make_unique<C>());
			e.ensureComponents<Ctail...>();
		}
	};

	// specialization to end the recursion
	template<> struct ensureComponents_Impl<> {
		static void ensure(Entity&) {		}
	};
}

template <class... C>
void Entity::ensureComponents() {
	ensureComponents_Impl<C...>::ensure(*this);
}


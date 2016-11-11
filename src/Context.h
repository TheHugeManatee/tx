#pragma once

#include "Entity.h"
#include "System.h"
#include "Identifier.h"

#include <vector>
#include <unordered_map>
#include <functional>


class Entity;
class System;

class Context {
public:
	Context() {};

	~Context() {};

	/// sets the entity with \a id, replacing a previous one if it existed
	void setEntity(const EntityID& id, std::unique_ptr<Entity> entity) {
		Entity& e = *entity;
		entities_[id] = std::move(entity);

		// TODO generate event
	}

	template<class S, typename... Args>
	void addSystem(Args... args) {
		// TODO check if already in there
		systems_.emplace_back(std::make_unique<S>(std::forward<Args>(args)...));
		systems_.back()->init(*this);
	}

	// implementation below
	template<typename ArrayN, typename Fn> 	void each(ArrayN cIds, Fn fn);

	void each(const std::function<void(const EntityID&, Entity&)> fn) {
		for (auto& e : entities_) {
			fn(e.first, *e.second);
		}
	}

	void notifyComponentChanged(const EntityID& eId, const ComponentID& cId) {
		// TODO generate event
		//std::cout << "Entity " << eId.name() << " changed Component " << cId.name();
	}

	template <typename C>
	C& getComponent(const EntityID& eId, const ComponentID& cId)
	{
		return (C&)*((entities_[eId]->components_).find(cId)->second); // is this actually a thing?? 
	}

	void updateSystems() {
		for (auto& s : systems_) {
			s->update(*this);
		}
	}

private:
	std::unordered_map<EntityID, std::unique_ptr<Entity>> entities_;
	std::vector<std::unique_ptr<System>> systems_;

	/// helper functions for variadic implementation
	
	template <typename ArrayN, typename Fn>
	struct each_variadic_impl
		: public each_variadic_impl<ArrayN, decltype(&Fn::operator())>
	{};

	// partial specialization on the lambda function
	template <size_t N, typename ClassType, typename FirstArgType, typename ReturnType, typename... C>
	struct each_variadic_impl<std::array<ComponentID, N>, ReturnType (ClassType::*) (FirstArgType, C...) const>
	{
		template <typename FFn>
		static void impl(Context& c, std::array<ComponentID, N> cIds, FFn fn) {
			
			static_assert(sizeof...(C) == N, "Number of Component IDs does not match functor signature!");

			size_t idx_tmp = 0;
			const auto aspect = Aspect<C...>(cIds);

			for (auto& e : c.entities_) {
				size_t idx = 0;
				if(aspect.hasAspect(*e.second))
					fn(e.first, c.getComponent<std::remove_reference<C>::type>(e.first, cIds[idx++])...);
			}
		};
	};
};

template<typename ArrayN, typename Fn>
void Context::each(ArrayN cIds, Fn fn) {
	each_variadic_impl<ArrayN, Fn>::impl(*this, cIds, fn);
}
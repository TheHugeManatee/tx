#pragma once

#include <assert.h>

#include <type_traits>

#include "Context.h"

class System {
public:
	System() {};
	virtual ~System() {};

	virtual bool isInterested(const Entity& ) { return true; };

	virtual void init(Context& c) = 0;
	virtual void update(Context& c) = 0;

	virtual void onEntityAdded(Context&, Entity& ) {};
	virtual void onEntityChanged(Context&, Entity& ) {};
	virtual void onEntityRemoved(Context&, Entity& ) {};

	
private:

};

// This template is needed so we can specialize it with a variadic template later
template <class A>
class AspectFilteringSystem : public System {};

// Specializing the class with variadic template
template <template<class... > class A, class... C>
class AspectFilteringSystem<A<C...>> : public System {
	template<class T> using add_ref = T&;

	virtual bool isInterested(Context&, const Entity& entity) override
	{ 
		return entity.hasAspect<C...>();
	};

	virtual void onEntityAdded(Context&, Entity& entity) override final {
		assert(entity.hasAspect<C...>());

		onEntityAdded((*entity.getComponent<C>())...);
	};

	virtual void onEntityAdded(Context&, add_ref<C>... components) = 0; // Subclasses should implement this instead of onEntityAdded(Entity&)
	//virtual void onEntityChanged(A::type_tuple components) {};
	//virtual void onEntityRemoved(A::type_tuple components) {};


};

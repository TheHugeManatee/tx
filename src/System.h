#pragma once

#include <assert.h>

#include <type_traits>

class Context;
class Entity;

class System {
public:
	System() {};
	virtual ~System() {};

	virtual bool isInterested(const Context&, const Entity&) { return true; };

	virtual void init(Context& c) {};
	virtual void update(Context& c) {};

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
public:
	template<class T> using add_ref = T&; // meta-programming for adding a reference, not very robust

	virtual bool isInterested(const Context&, const Entity& entity) override
	{ 
		//return entity.hasAspect<C...>();
		return true;
	};

	virtual void onEntityAdded(Context& ctxt, Entity& entity) override final {
		//assert(entity.hasAspect<C...>());

		//onEntityAdded(ctxt, (*entity.getComponent<C>())...);
	};

	virtual void onEntityAdded(Context&, add_ref<C>... components) = 0; // Subclasses should implement this instead of onEntityAdded(Entity&)
	//virtual void onEntityChanged(A::type_tuple components) {};
	//virtual void onEntityRemoved(A::type_tuple components) {};


};

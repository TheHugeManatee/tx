#pragma once

#include <assert.h>

#include <type_traits>

class Context;
class Entity;

class System {
public:
	System() {};
	virtual ~System() {};

	virtual bool isInterested(const Context&, const EntityID&, const Entity&) { return true; };

	virtual void init(Context& ) {};
	virtual void update(Context& ) {};

	virtual void onEntityAdded(Context&, const EntityID& ) {};
	virtual void onEntityChanged(Context&, const EntityID& ) {};
	virtual void onEntityRemoved(Context&, const EntityID& ) {};

	
private:

};

// This template is needed so we can specialize it with a variadic template later
template <class Aspect_>
class AspectSpecificSystem : public System {};

// Specializing the class with variadic template
template <template<class... > class Aspect_, class... C>
class AspectSpecificSystem<Aspect_<C...>> : public System {
public:
	const size_t nCmp = sizeof...(C);
	template<class T> using add_ref = T&; // meta-programming for adding a reference, probably should use something from STL instead...

	AspectSpecificSystem(const std::array<ComponentID, sizeof...(C)>& cIds) 
		: aspect(cIds)
	{	}

	virtual bool isInterested(const Context&, const EntityID& eId, const Entity& entity) override;

	virtual void onEntityAdded(Context& ctxt, const EntityID& eId) override final;

	/// Subclasses have to implement this instead of onEntityAdded(Context&,EntityID&)
	virtual void onEntityAdded(Context&, const EntityID&, C&... components) = 0; 
	
	//virtual void onEntityChanged(A::type_tuple components) {};
	
	//virtual void onEntityRemoved(A::type_tuple components) {};

private:
	Aspect<C...> aspect;
};

#include "Context.h"

template <template<class... > class Aspect_, class... C>
void AspectSpecificSystem<Aspect_<C...>>::onEntityAdded(Context& ctxt, const EntityID& eId)
{
	assert(aspect.hasAspect(ctxt.getEntity(eId)));

	size_t idx = 0;
	onEntityAdded(ctxt, eId, ctxt.getComponent<C>(eId, aspect.ids_[idx++])...);
}


template <template<class... > class Aspect_, class... C>
bool AspectSpecificSystem<Aspect_<C...>>::isInterested(const Context&, const EntityID& eId, const Entity& entity)
{
	return aspect.hasAspect(entity);
}

#pragma once

#include <assert.h>

#include <type_traits>

class Context;
class Entity;
struct Event;

template<typename... C> class Aspect;

class System {
public:
    System() : invalid_{ true } {};
	virtual ~System() {};

	virtual bool isInterested(const Context&, const EntityID&) { return false; };

	virtual void init(Context& ) {};
    virtual bool update(Context&) { return false; }; // updates the system. return whether it is now in a valid state or not
    virtual void onEvent(const Event& ) {};

    void setInvalid() {  invalid_ = true;    }
    void setValid()   {  invalid_ = false;   }
    bool isValid()    {  return !invalid_;   }

private:
    bool invalid_;
};

// This template is needed so we can specialize it with a variadic template later
template <class Aspect_>
class AspectSpecificSystem : public System {};

// Specializing the class with variadic template
template <template<class... > class Aspect_, class... C>
class AspectSpecificSystem<Aspect_<C...>> : public System {
public:
//	const size_t nCmp = sizeof...(C);

	AspectSpecificSystem(const std::array<ComponentID, sizeof...(C)>& cIds) 
		: aspect(cIds)
	{	}

    virtual ~AspectSpecificSystem() {
    }

	virtual bool isInterested(const Context&, const EntityID& eId) override;
    
private:
	Aspect<C...> aspect;
};


#include "Context.h"

template <template<class... > class Aspect_, class... C>
bool AspectSpecificSystem<Aspect_<C...>>::isInterested(const Context& c, const EntityID& eId)
{
	return aspect.checkAspect(c.getEntity(eId));
}

#pragma once

#include <atomic>
#include <mutex>
#include <queue>
#include <type_traits>

#include "Event.h"

namespace tx
{

class Context;
class Entity;

/**
 *  Polymorphic base class for Systems.
 *
 *  This is the common base class for systems. Do not derive from this
 *  directly but rather derive from the templated \a System class in order to leverage CRTP.
 */
class SystemBase
{
public:
    SystemBase(bool valid = false) : valid_{valid} {};

    virtual ~SystemBase(){};

    /**
     *  [Threadsafe] Tests whether the system is interested in updates regarding a specific entity.
     *
     *  By default, don't care about any entities.
     */
    virtual bool isInterested(const Context&, const EntityID&, const ComponentID&) const
    {
        return false;
    };

    /**
     *  Tests whether the system is interested in events regarding a specific system [Threadsafe].
     *
     *  By default, don't care about any other systems.
     */
    virtual bool isInterested(const SystemID&) const { return false; };

    /**
     *  Initialize the system. Should only be called once, by the context.
     */
    virtual void init(Context&){};

    /**
     *  virtual function to get the Unique ID of the system.
     */
    virtual SystemID getID() = 0;

    /**
     *  Update the system.
     *
     *  Does the actual "work" of the system. Base class implementation is the bare minimum:
     *  "Process" all events and state that the system is now valid.
     *
     *  \return     whether the processor is now in a valid state or if
     *              it should be updated again.
     */
    virtual bool update(Context&)
    {
        clearEventQueue();
        return true;
    };

    /**
     *  [Threadsafe] Pushes an event to the system's event queue.
     *
     *  We use a back-buffered queue to increase performance of this call:
     *  While the front queue is blocked inside processEvents(), pushed events are enqueued in the
     * back queue to prevent blocking other threads while processing long event lists.
     */
    void pushEvent(const Event& e)
    {
        if (!frontQueueProcessing_) {
            std::lock_guard<std::mutex> eqLock(eventQueueMutex_);
            eventQueue_.push(e);
        }
        else
        {
            std::lock_guard<std::mutex> ebqLock(eventBackQueueMutex_);
            backEventQueue_.push(e);
        }
        setInvalid();
    };

    /**
     *  [Threadsafe] Process all events in the event queue and clears the event queue.
     *
     *  Note that, due to multithreading, after this call the event queue is not
     *  necessarily empty, as during the processing of the back queue other threads
     *  might add events to the front queue again.
     */
    template<typename FFn>
    void processEvents(FFn fn)
    {
        { // Process all events from the main queue
            SET_TEMPORARILY(frontQueueProcessing_, true);
            std::lock_guard<std::mutex> eqLock(eventQueueMutex_);
            while (!eventQueue_.empty())
            {
                fn(eventQueue_.front());
                eventQueue_.pop();
            }
        }
        { // Process all events from the back queue. This part will block potential pushEvent()
          // calls until complete,
            // but should be over rather fast
            std::lock_guard<std::mutex> ebqLock(eventBackQueueMutex_);
            while (!backEventQueue_.empty())
            {
                fn(backEventQueue_.front());
                backEventQueue_.pop();
            }
        }
    }

    /**
     *  [Threadsafe] Clears the event queue of the system.
     *
     *  Simply dequeues all events and discards them. Can be called as an
     *  alternative to \a processEvents if they are not interesting.
     */
    void clearEventQueue()
    {
        { // clear all events from the main queue
            SET_TEMPORARILY(frontQueueProcessing_, true);
            std::lock_guard<std::mutex> eqLock(eventQueueMutex_);
            while (!eventQueue_.empty())
            {
                eventQueue_.pop();
            }
        }
        { // clear all events from the back queue
            std::lock_guard<std::mutex> ebqLock(eventBackQueueMutex_);
            while (!backEventQueue_.empty())
            {
                backEventQueue_.pop();
            }
        }
    }

    /**
     *  Sets the system invalid.
     */
    void setInvalid() { valid_ = false; }

    /**
     *  Sets the system valid
     */
    void setValid() { valid_ = true; }

    /**
     *  Checks whether the system is valid
     */
    bool isValid() const { return valid_; }

private:
    std::atomic<bool>
        valid_; ///< Flag to indicate whether the system is currently valid or if it needs to update()

    // threadsafe event queue
    std::atomic<bool>
                      frontQueueProcessing_; ///< atomic indicating when the system is currently processing the front event queue
    std::queue<Event> eventQueue_;
    std::mutex        eventQueueMutex_;
    std::queue<Event> backEventQueue_;
    std::mutex        eventBackQueueMutex_;
};

template<typename Derived>
class System : public SystemBase
{
public:
    System() : SystemBase(){};
    explicit System(bool valid) : SystemBase(valid){};

    SystemID getID() final { return id(); };

    static SystemID id()
    {
        SystemID t(typeid(Derived));
        t.id_[3] = typeid(Derived).hash_code();
        return t;
    };
};

// Forward declaration
template<typename... C>
class Aspect;

// This template is needed so we can specialize it with a variadic template later
template<class Aspect_>
class AspectSpecificSystem : public System<AspectSpecificSystem<Aspect_>>
{
};

// Specializing the class with variadic template
template<template<class...> class Aspect_, class... C>
class AspectSpecificSystem<Aspect_<C...>> : public System<AspectSpecificSystem<Aspect_<C...>>>
{
public:
    //	const size_t nCmp = sizeof...(C);

    AspectSpecificSystem(std::array<ComponentID, sizeof...(C)>&& cIds, bool valid = false)
        : System<AspectSpecificSystem<Aspect_<C...>>>(valid), aspect(std::move(cIds))
    {
    }

    virtual ~AspectSpecificSystem() {}

    virtual bool isInterested(const Context&, const EntityID& eId,
                              const ComponentID& cId) const override;

private:
    Aspect<C...> aspect;
};

} // namespace tx

#include "Context.h"

namespace tx
{

template<template<class...> class Aspect_, class... C>
bool AspectSpecificSystem<Aspect_<C...>>::isInterested(const Context& /*c*/,
                                                       const EntityID& /*eId*/,
                                                       const ComponentID& cId) const
{
    // TODO restore this check somehow
    return aspect.isIDPartOf(cId) /* && aspect.checkAspect(c.getEntity(eId))*/;
}

} // namespace tx

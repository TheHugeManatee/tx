#pragma once

#include <atomic>
#include <queue>
#include <mutex>
#include <type_traits>

#include "Event.h"

namespace tx {

    class Context;
    class Entity;

    template<typename... C> class Aspect;

    class System {
    public:
        explicit System(bool valid = false) : valid_{ valid } {};
        virtual ~System() {};

        // By default, don't care about any events
        virtual bool isInterested(const Context&, const EntityID&) const { return false; };

        virtual void init(Context&) {};
        virtual bool update(Context&) { return true; }; // updates the system. return whether it is now in a valid state or not

        /**
         *  Pushes an event to the system's event queue.
         *  We use a back-buffered queue to increase performance of this call:
         *  While the front queue is blocked inside processEvents(), pushed events are enqueued in the back queue to prevent
         *  blocking other threads while processing long event lists.
         */
        void pushEvent(const Event& e) {
            if (!frontQueueProcessing_) {
                std::lock_guard<std::mutex> eqLock(eventQueueMutex_);
                eventQueue_.push(e);
            }
            else {
                std::lock_guard<std::mutex> ebqLock(eventBackQueueMutex_);
                backEventQueue_.push(e);
            }
            setInvalid();
        };


        /**
         *  Process all events in the event queue and clears the event queue. Note that, due to multithreading,
         *  after this call the event queue is not necessarily empty, as during the processing of the back queue
         *  other threads might add events to the front queue again.
         */
        template <typename FFn>
        void processEvents(FFn fn) {
            {   // Process all events from the main queue
                SET_TEMPORARILY(frontQueueProcessing_, true);
                std::lock_guard<std::mutex> eqLock(eventQueueMutex_);
                while (!eventQueue_.empty()) {
                    fn(eventQueue_.front());
                    eventQueue_.pop();
                }
            }
            {   // Process all events from the back queue. This part will block potential pushEvent() calls until complete, 
                // but should be over rather fast
                std::lock_guard<std::mutex> ebqLock(eventBackQueueMutex_);
                while (!backEventQueue_.empty()) {
                    fn(backEventQueue_.front());
                    backEventQueue_.pop();
                }
            }
        }

        void clearEventQueue() {
            {   // clear all events from the main queue
                SET_TEMPORARILY(frontQueueProcessing_, true);
                std::lock_guard<std::mutex> eqLock(eventQueueMutex_);
                while (!eventQueue_.empty()) {
                    eventQueue_.pop();
                }
            }
            {   // clear all events from the back queue
                std::lock_guard<std::mutex> ebqLock(eventBackQueueMutex_);
                while (!backEventQueue_.empty()) {
                    backEventQueue_.pop();
                }
            }
        }

        void setInvalid() { valid_ = false; }
        void setValid() { valid_ = true; }
        bool isValid() { return valid_; }

    private:
        std::atomic<bool> valid_; ///< Flag to indicate whether the system is currently valid or if it needs to update()

        // threadsafe event queue
        std::atomic<bool> frontQueueProcessing_; ///< atomic indicating when the system is currently processing the front event queue
        std::queue<Event> eventQueue_;
        std::mutex eventQueueMutex_;
        std::queue<Event> backEventQueue_;
        std::mutex eventBackQueueMutex_;
    };

    // This template is needed so we can specialize it with a variadic template later
    template <class Aspect_>
    class AspectSpecificSystem : public System {};

    // Specializing the class with variadic template
    template <template<class... > class Aspect_, class... C>
    class AspectSpecificSystem<Aspect_<C...>> : public System {
    public:
        //	const size_t nCmp = sizeof...(C);

        AspectSpecificSystem(const std::array<ComponentID, sizeof...(C)>& cIds, bool valid = false)
            : System(valid)
            , aspect(cIds)
        {	}

        virtual ~AspectSpecificSystem() {
        }

        virtual bool isInterested(const Context&, const EntityID& eId) const override;

    private:
        Aspect<C...> aspect;
    };

} // namespace tx

#include "Context.h"

template <template<class... > class Aspect_, class... C>
bool tx::AspectSpecificSystem<Aspect_<C...>>::isInterested(const Context& c, const EntityID& eId) const
{
    return aspect.checkAspect(c.getEntity(eId));
}

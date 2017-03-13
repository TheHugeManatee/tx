#pragma once

/**
 *  Event class signaling that some TX event has occurred.
 *  Stores the ID of the related entity, as well as either the related second entity or the component.
 */
struct Event {
    enum EventType {
        COMPONENTADDED,
        COMPONENTCHANGED,
        COMPONENTREMOVED,
        ENTITYCREATED,
        ENTITYREMOVED
    };

    Event(EventType type_, const EntityID& eId0_, const EntityID& eId1_) :
        type(type_), eId(eId0_), eId1(eId1_) {};
    Event(EventType type_, const EntityID& eId_, const ComponentID& cId_) :
        type(type_), eId(eId_), cId(cId_) {};

    Event(Event&& rhs) = default;
    Event& operator=(Event&& rhs) = default;
    Event(const Event& rhs) = default;
    Event& operator=(const Event& rhs) = default;

    EventType type;
    EntityID eId;
    union {
        ComponentID cId;
        EntityID eId1;
    };
};


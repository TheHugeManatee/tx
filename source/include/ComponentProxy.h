#pragma once

#include "Component.h"
#include "Context.h"

namespace tx {

    template <typename T>
    class ComponentProxy {
    public:
        constexpr ComponentProxy(Context & ctxt_, const EntityID & eId_, const ComponentID & cId_, T & comp_)
            : ctxt(ctxt_), eId(eId_), cId(cId_), comp(comp_)
        {
        }

        constexpr ~ComponentProxy() {
            // signal the context that the component was 
            ctxt.notifyComponentChanged(eId, cId);
        }

        // TODO figure out how to implement C++ proxy properly
        //T& operator*->() {
        //	return comp;
        //}

        operator T() {
            return comp;
        }

    private:
        Context & ctxt;
        const EntityID & eId;
        const ComponentID & cId;
        T & comp;
    };

} // namespace tx

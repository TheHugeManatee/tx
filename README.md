 # TX ( = Templated ECS - tecs)
 ## What is it?
 TX is a simple, lightweight, header-only library for implementing entity-component-system based architectures.

 Its focus is on
  - Simplicity in design
  - Easy integration into existing code bases
  - Flexible component types and named components
  - It can be used as an interoperability layer between different algorithms
  - It supports parallelism
  - A pronounced goal is to handle all data of the application, i.e. state as well as assets in memory

Therefore,
  - It is optimized for flexible, dynamic entities
  - It deals with heterogeneous entities instead of optimizing for homogeneous entities having the same components. For these scenarios, refer to other ECS libraries such as ECST or EntityX
  - It is designed to support handling of low (tens of) thousands of individual entities, not millions


## Definitions
The following key concepts are defined and mostly correspond to actual classes in the framework.

### Entity
An entity defines a generic "object" in the framework. It can correspond to a physical object but conceptually, it is just a collection of components that can be dynamically added, changed and removed.

Entities simply encapsulate related data fields by grouping them together under a certain name (Entity ID), without any notion of logic.

### Component
A component is a part of an entity. It represents a data field related to some specific semantic knowledge of the entity.
By design of the library, any external, user-defined type can be wrapped as a component and attached to an entity. 

As with Entities themselves, components are tagged with a name (Component ID) and should not have any logic associated with them.

### Aspect
An aspect of an entity can be thought of as an interface specification that an entity suffices or not. Therefore, an aspect is a specification of Component-ID + Component Type pairs that an entity can be checked against to ascertain if an entity has that aspect.

### Event
Events are created for specific changes in the context. Currently, the most important events are:
 - **ComponentChanged**: Notifies an update to a component, passing both the name of the entity and the component that have changed
 - **SystemUpdated**: Notifies that a system update() has finished.

### System
A system encapsulates a specialized part of the program logic.
They are expected to be stateless and only work on the data attached to the entities, potentially outputting new entities or components or modifying existing components.
Systems implement logic that generally operates on all entities that have a specific aspect that is interesting for the system.

Systems are generally inactive as long as they are in a valid state.
If a system is invalidated, it will be scheduled to update using an internal thread pool. A system can be either invalidated externally by client code, or internally by receiving an Event it is interested in.

A system specifies which Events it is interested in by overriding one or several isInterested overloads.
Therefore, there are three ways to semantically trigger a system:
 - **Component Update triggered**: Systems can be triggered when specific components have changed. These component change updates can be filtered to only trigger on components of entities which define specific aspects, which is the most common use case. This can be for example independent subsystems that trigger irregularly when high-level parameters change, for example the window size or a global setting.
 - **Sequentially triggered** : A system is triggered to update if one or several upstream systems have finished updating. This closely corresponds to nodes in a data flow network, which only execute if their input data has changed.
- **Externally triggered**: Client code issues an update for the system.

Dependencies can be specified between systems to optimize the parallel or sequential execution.

Note that Systems in general are not intended to operate on only one specific entity, but process all entities that have a certain aspect. In practice, systems often define special "tag" components that are attached to the entity that it operates on to simulate such a behavior.

### Context
The context is the central object which handles Systems, entities and components.
It consists of a list of current entities and systems, and manages execution of the systems.

 - It provides an API for thread-safe access to the entities and components, both as an interface to the active systems as well as a client application.
 - When components have been changed, it will automatically invoke (parallel) execution of all dependent event-based systems

## Illustrating example 1 - Particle Simulation
As a simple example often used as a demonstration for ECS, a simple particle simulation shall be considered.
Particles are points in 2D space with a radius, colliding with each other and the boundaries of the simulation environment.

### Entities and Components
Entities here will mainly consist of two components, Position x and Velocity v.
Additionally, GPU Textures are stored as components.

### Systems
The simulaton can be decomposed into different systems:
 - **Collision system**: Timed System that checks all pairs of particles for a collision and adjusts velocities accordingly
 - **Acceleration System**: Sequential System after Collision which dds a constant acceleration ("gravity") to every particle as v := v + dT * a;
 - **Movement System**: Sequential system afer Acceleration that updates the position according to x := x + dT * v for every component
 - **Border Constraint System**: Sequential system after movement system that checks the simulation boundaries, deflecting velocities and re-positioning particles that are outside
 - **Render Preparation System**: Timed System that copies position, velocity and radius data from all particles into a buffer for asynchronous rendering. Outputs the buffer as a component of a "Rendering" entity
 - **Rendering System**: Sequential system that reads the buffered data, posts uploads it to the GPU and renders to a texture, which is in turn output as another Component of the "Rendering" entity
 - **Display System**: Event-based system that reacts on changes of the texture of the "Rendering" component and displays the texture to the screen using the OS's window manager.

 Note that there are a lot of ways to vary the design, like combining several of the simulation systems into one system or combining the rendering systems.

## Illustrating Example 2 - Simple AR Application
Another potential use case is the implementation of an AR application.
This application uses a camera stream, detects markers in the camera image and renders geometry on top of which is aligned with the markers.
For this example, we assume the client application only uses TX as as an interop library for some features; providing the camera image and retrieving the rendered and composed image for further processing.

### Entities and Components
Entities do not have any common properties in general in this example.

### Systems
 - **Marker Detector**: Event-based system that triggers everytime the image component of "CameraImage" entity has changed. Reads camera calibration parameters from the Components of "CameraImage" and runs the marker detection. Outputs the detected marker pose into separate entities for every marker.
 - **Geometry Creator**: Event-based system that reacts on some configuration entity to create the geometry data that should be rendered on the markers, i.e. a cube. Attaches the geometry to all marker entities
 - **Geometry Renderer**: Event-based, reacts on changes to entities which have an aspect that defines the pose as well as an associated geometry. Renders all instances of that aspect into a common render target, using the camera image as a background (read from the CameraImage entity). Outputs the result into a texture component.
 - **Texture Retriever**: Event-based, waits for the texture generated by the Geometry Renderer and copies/transfers it outside the TX context to be used further by the client application.


 ## TODO's / Big Question Marks
  - How can component links / references be implemented?
  - Is it possible to implement hierarchical structure into the enties, i.e. a scenegraph-like structure?
  - Namespacing!

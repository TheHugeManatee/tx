#include "Entity.h"
#include "Component.h"
#include "System.h"
#include "Aspect.h"
#include "Context.h"
#include "Identifier.h"

#include <vector>
#include <iostream>
#include <typeindex>

/// ======================== Some Classes to manage ========================
struct Vec3 {
    double x, y, z;
    Vec3(double x_ = 0.0, double y_ = 0.0, double z_ = 0.0) : x(x_), y(y_), z(z_) {};
};

/// ======================== Some component definitions ========================

//class PositionCmp : public Component<Vec3> {};
//class VelocityCmp : public Component<Vec3> {};
//struct meshCl {
//	std::vector<Vec3> vertices;
//	std::vector<size_t> indices;
//};
//class MeshCmp : public Component<meshCl> {};
//class TagCmp : public Component<TagID>{};

using PositionCmp = Component<Vec3>;
using VelocityCmp = Component<Vec3>;
struct meshCl {
    std::vector<Vec3> vertices;
    std::vector<size_t> indices;
};
using MeshCmp = Component<meshCl>;
using TagCmp = Component<TagID>;


/// Generating an aspect - different API possibilities, not sure which one is the cleanest
// generate aspect instance using make_aspect
//const auto simAspect = make_aspect(std::make_pair(ComponentID("Position"), PositionCmp()), std::make_pair(ComponentID("Velocity"), VelocityCmp()));
// get type of the aspect
using SimAspect = Aspect<PositionCmp, VelocityCmp>;
// generate aspect instance using initializer list
const SimAspect simAspect({ "Position", "Velocity" });
// generate aspect instance using template arguments
const Aspect<PositionCmp, VelocityCmp> simAspect3({ "Position", "Velocity" });

using DrawAspect = Aspect<PositionCmp, MeshCmp>;
const DrawAspect drawAspect({ "Position", "Mesh" });
const Aspect<PositionCmp, VelocityCmp, MeshCmp> allAspect({ "Position", "Velocity", "Mesh" });

/// ======================== defining some systems ========================

class SetupSystem : public System {
    bool update(Context &c) override {
        c.setComponent<PositionCmp>("config", "origin", 0., 0., 0.);
        c.setComponent<PositionCmp>("config", "direction");
        c.setComponent<VelocityCmp>("config", "gravity", 0., 0., -9.81);
        return true;
    }
};

class DrawingSystem : public AspectSpecificSystem<DrawAspect> {
public:
	DrawingSystem() : AspectSpecificSystem<DrawAspect>(std::array<ComponentID, 2>{ "Position", "Mesh" }) {};

	bool update(Context& c) override {
		std::cout << "Drawing system update(): " << std::endl;
		c.each(std::array<ComponentID, 2>{ "Position", "Mesh" }, [&c](const EntityID& id, const PositionCmp& pos, const MeshCmp& m) -> void {
			std::cout << "\t Drawing " << id << " with " << m->vertices.size() << " vertices at " << pos->x << " " << pos->y << " " << pos->z << std::endl;
		});
        return true;
	}

    void onEvent(const Event& e) override {
        std::cout << "\t\t\tDrawing System got an event about " << e.eId << std::endl;
        setInvalid();
    }
};

class SimulationSystem : public System {
public:
	bool update(Context& c) override {
		std::cout << "Simulation System update(): " << std::endl;
		c.each(std::array<ComponentID, 2>{ "Position", "Velocity" }, [&c](const EntityID& id, PositionCmp& pos, const VelocityCmp& v) -> void {
            pos->x += v->x;
            pos->y += v->y;
            pos->z += v->z;
			std::cout << "\tMoving " << id << " to " << pos->x << " " << pos->y << " " << pos->z << std::endl;
		});
        return false;
	}

    void onEvent(const Event& e) override {
        std::cout << "\t\t\tSimulation System got an event about " << e.eId << std::endl;
    }
};

class UpdaterSystem : public System {
public:
	bool update(Context& c) override {
		std::cout << "Updater update(): " << std::endl;
		c.each([&c](const EntityID& id, Entity& e) {
			std::cout << "\tUpdating " << id << std::endl;
		});
        return false;
	}

    void onEvent(const Event& e) override {
        std::cout << "\t\t\tUpdating System got an event about " << e.eId << std::endl;
    }
};


/// ======================== the main function ========================
int main(int, char*[]) {

	std::cout << std::endl << "------------------------------------------------------------------" << std::endl;

	Entity cube;
	cube.setComponent("Position", std::make_unique<PositionCmp>(1.,1.,1.));
	cube.setComponent("Velocity", std::make_unique<VelocityCmp>(2.,0.,0.));

    Entity circle;
	circle.setComponent("Position", std::make_unique<PositionCmp>(2.,2.,2.));
	circle.setComponent("Velocity", std::make_unique<VelocityCmp>(0.,2.,0.));

    Entity foo;
	foo.setComponent("Position", std::make_unique<PositionCmp>(3.,3.,3.));
	foo.setComponent("Velocity", std::make_unique<VelocityCmp>(0.,0.,-2.));
	foo.setComponent("Mesh", std::make_unique<MeshCmp>());

	std::cout << std::endl << "------------------------------------------------------------------" << std::endl;

	Context world;

    world.addSystem<SetupSystem>();
	world.addSystem<SimulationSystem>();
	world.addSystem<UpdaterSystem>();
    world.addSystem<DrawingSystem>();

    world.setEntity("cube", std::move(cube));
    world.setEntity("circle", std::move(circle));
    world.setEntity("foo", std::move(foo));

	std::cout << std::endl << "------------------------------------------------------------------" << std::endl;
	std::cout << "Updating the world.." << std::endl;
    world.runSequential([]() { static int t = 0; return ++t < 3; });

	std::cout << std::endl << "------------------------------------------------------------------" << std::endl;

	std::cout << std::endl << "------------------------------------------------------------------" << std::endl;
	std::cout << std::endl << std::endl << "\t\t\t~~~ Fin. ~~~" << std::endl << std::endl;
}
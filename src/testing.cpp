#include "Entity.h"
#include "Component.h"
#include "System.h"
#include "Aspect.h"
#include "Context.h"
#include "Identifier.h"
#include "utils.h"

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

/// ======================== various helper and test functions ========================
template<typename T> void testSizeof() {
    std::cout << "Size of " << typeid(T).name() << ": " << sizeof(T) << std::endl;
}

template<typename T> void testSizeof(const T&) {
    std::cout << "Size of " << typeid(T).name() << " [" << std::type_index(typeid(T)).hash_code() << "]: " << sizeof(T) << std::endl;
}

template <size_t N>
std::array<char, N> mk(std::array<char, N> l) {
    return std::array<char, N>(l);
}

std::string tf(bool b) {
    return b ? "true" : "false";
}

uint64_t hashTest(Identifier i) {
    return i.hash();
}

std::string idName(const ComponentID& id) {
    return "C" + id.name();
}

std::string idName(const EntityID& id) {
    return "E" + id.name();
}

std::string idName(const Identifier& id) {
    return "I" + id.name();
}

#include <unordered_map>

/// ======================== the main function ========================
int main(int, char*[]) {

    std::cout << std::endl << "------------------------------------------------------------------" << std::endl;

    Entity cube;
    cube.setComponent("Position", std::make_unique<PositionCmp>(1., 1., 1.));
    cube.setComponent("Velocity", std::make_unique<VelocityCmp>(2., 0., 0.));

    Entity circle;
    circle.setComponent("Position", std::make_unique<PositionCmp>(2., 2., 2.));
    circle.setComponent("Velocity", std::make_unique<VelocityCmp>(0., 2., 0.));

    Entity foo;
    foo.setComponent("Position", std::make_unique<PositionCmp>(3., 3., 3.));
    foo.setComponent("Velocity", std::make_unique<VelocityCmp>(0., 0., -2.));
    foo.setComponent("Mesh", std::make_unique<MeshCmp>());

    std::cout << std::endl << "------------------------------------------------------------------" << std::endl;

    using one = std::integral_constant<int, 1>;
    using two = std::integral_constant<int, 2>;
    using five = std::integral_constant<int, two::value + two::value + one::value>;

    std::cout << typeid(one).name() << std::endl;
    std::cout << typeid(two).name() << std::endl;
    std::cout << typeid(five).name() << std::endl;

    std::cout << std::endl << "------------------------------------------------------------------" << std::endl;

    std::cout << "\t[cube]" << std::endl;
    std::cout << "Has Position: " << tf(cube.hasComponent<PositionCmp>("Position")) << std::endl;
    std::cout << "Has Velocity: " << tf(cube.hasComponent<VelocityCmp>("Velocity")) << std::endl;
    std::cout << "Has Mesh: " << (cube.hasComponent<MeshCmp>("Mesh") ? "true" : "false") << std::endl;
    std::cout << "Has Simulation Aspect: " << (simAspect.checkAspect(cube) ? "true" : "false") << std::endl;
    std::cout << "Has Drawing Aspect: " << (drawAspect.checkAspect(cube) ? "true" : "false") << std::endl;

    std::cout << "\t[circle]" << std::endl;
    std::cout << "Has Position: " << (circle.hasComponent<PositionCmp>("Position") ? "true" : "false") << std::endl;
    std::cout << "Has Velocity: " << (circle.hasComponent<VelocityCmp>("Velocity") ? "true" : "false") << std::endl;
    std::cout << "Has Mesh: " << (circle.hasComponent<MeshCmp>("Mesh") ? "true" : "false") << std::endl;
    std::cout << "Has Simulation Aspect: " << (simAspect.checkAspect(circle) ? "true" : "false") << std::endl;
    std::cout << "Has Drawing Aspect: " << (drawAspect.checkAspect(circle) ? "true" : "false") << std::endl;

    std::cout << "\t[foo]" << std::endl;
    std::cout << "Has Position: " << (foo.hasComponent<PositionCmp>("Position") ? "true" : "false") << std::endl;
    std::cout << "Has Velocity: " << (foo.hasComponent<VelocityCmp>("Velocity") ? "true" : "false") << std::endl;
    std::cout << "Has Mesh: " << (foo.hasComponent<MeshCmp>("Mesh") ? "true" : "false") << std::endl;
    std::cout << "Has Simulation Aspect: " << (simAspect.checkAspect(foo) ? "true" : "false") << std::endl;
    std::cout << "Has Drawing Aspect: " << (drawAspect.checkAspect(foo) ? "true" : "false") << std::endl;

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

    testSizeof<char*>();
    testSizeof<std::unique_ptr<char>>();
    testSizeof<std::shared_ptr<char>>();
    testSizeof<uint64_t>();
    testSizeof<size_t>();
    testSizeof<int>();

    std::cout << std::endl << "------------------------------------------------------------------" << std::endl;
    // testing the identifier function
    std::array<char, sizeof("hello world!")> s{ "hello world!" };
    std::array<char, sizeof("blabla")> S = { "blabla" };
    //std::array<char, sizeof("hello world! ")> s2{ s,'h' };

    CONSTEXPR Identifier i("blabla");
    CONSTEXPR Identifier j("blablu");
    CONSTEXPR Identifier k("ABCDEFGHIJKLMNOPQRSTUVWXYZ1234");
    CONSTEXPR Identifier l("ABCDEFGHIJKLMNOPQRSTUVWXYZ12345");
    CONSTEXPR Identifier m("ABCDEFGHIJKLMNOPQRSTUVWXYZ123456");
    CONSTEXPR Identifier n("ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567");
    CONSTEXPR bool ieqj = i == j;
    //constexpr uint64_t ihash{ i.hash() }; // somehow this does not run in VS, might be compiler bug
    CONSTEXPR bool iltj = i < j;

    std::cout << "Identifier i: " << i << " " << i.hash() << std::endl;
    std::cout << "Identifier j: " << j << " " << j.hash() << std::endl;
    std::cout << "Identifier k: " << k << " " << k.hash() << std::endl;
    std::cout << "Identifier l: " << l << " " << l.hash() << std::endl;
    std::cout << "Identifier m: " << m << " " << m.hash() << std::endl;
    std::cout << "Identifier n: " << n << " " << n.hash() << std::endl;

    std::cout << "i == j: " << tf(ieqj) << std::endl;
    std::cout << "i == k: " << tf(i == k) << std::endl;
    std::cout << "k == l: " << tf(k == l) << std::endl;
    std::cout << "l == m: " << tf(l == m) << std::endl;
    std::cout << "m == n: " << tf(m == n) << std::endl;
    std::cout << "i < j: " << tf(iltj) << std::endl;

    // testing hash function
    std::unordered_map<Identifier, std::shared_ptr<ComponentBase>> map;
    map[i] = std::make_shared<PositionCmp>();
    map[j] = std::make_shared<PositionCmp>();

    std::cout << "Hash of 's' is " << hashTest("s") << std::endl; // test implicit identifier instantiation

    std::cout << std::endl << "------------------------------------------------------------------" << std::endl;
    // some sizeof experiments
    testSizeof(typeid(std::unordered_map<int, int>));
    testSizeof(std::type_index(typeid(std::unordered_map<int, int>)));
    testSizeof<ComponentID>();
    testSizeof<EntityID>();

    std::cout << "Strong typedef test: " << idName(Identifier("Identifier")) << ", " << idName(ComponentID("ComponentID")) << ", " << idName(EntityID("EntityID")) << std::endl;


    std::cout << std::endl << "------------------------------------------------------------------" << std::endl;
    std::cout << std::endl << std::endl << "\t\t\t~~~ Fin. ~~~" << std::endl << std::endl;
}
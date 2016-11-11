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
};

/// ======================== Some component definitions ========================
class NameCmp : public Component {

};

class PositionCmp : public Component {
public:
	Vec3 value;
};

class VelocityCmp : public Component {
public:
	Vec3 value;
};

class MeshCmp : public Component {
public:
	std::vector<Vec3> vertices;
	std::vector<size_t> indices;
};

class TagCmp : public Component {
	TagCmp(const TagID& tag_) : tag(tag_) {};
	const TagID tag;
};

/// Generating an aspect - different API possibilities, not sure which one is the cleanest
// generate aspect instance using make_aspect
const auto simAspect = make_aspect(std::make_pair(ComponentID("Position"), PositionCmp()), std::make_pair(ComponentID("Velocity"), VelocityCmp()));
// get type of the aspect
using SimAspect = decltype(simAspect);
// generate aspect instance using initializer list
const SimAspect simAspect2({ "Position", "Velocity" });
// generate aspect instance using template arguments
const Aspect<PositionCmp, VelocityCmp> simAspect3({ "Position", "Velocity" });

using DrawAspect = Aspect<PositionCmp, MeshCmp>;
const DrawAspect drawAspect({ "Position", "Mesh" });
const Aspect<PositionCmp, VelocityCmp, MeshCmp> allAspect({ "Position", "Velocity", "Mesh" });

/// ======================== defining some systems ========================

class DrawingSystem : public AspectFilteringSystem<DrawAspect> {
public:
	void onEntityAdded(Context&, PositionCmp&, MeshCmp&) override {
		std::cout << "Drawing System noticed a new Entity!" << std::endl;
	}
	void update(Context& c) override {
		std::cout << "Drawing system processes: " << std::endl;
		c.each(std::array<ComponentID, 2>{ "Position", "Mesh" }, [&c](const EntityID& id, PositionCmp&, MeshCmp&) -> void {
			std::cout << "\t" << id.name() << std::endl;// << "\t" << e.stringify();
		});
	}
};

class SimulationSystem : public AspectFilteringSystem<SimAspect> {
public:
	//void onEntityAdded(Context&, PositionCmp&, VelocityCmp&) override {
	//	std::cout << "Simulation System noticed a new Entity!" << std::endl;
	//}
	void update(Context& c) override {
		std::cout << "Simulation System finds: " << std::endl;
		c.each(std::array<ComponentID, 2>{ "Position", "Velocity" }, [&c](const EntityID& id, PositionCmp&, MeshCmp&) -> void {
			std::cout << "\t" << id.name() << std::endl;
		});
	}
};

class UpdaterSystem : public System {
public:
	void onEntityAdded(Context&, Entity& e) override {
		std::cout << "Updater System noticed a new Entity: " << e.stringify() << std::endl;
	}

	void update(Context& c) override {
		std::cout << "Updater finds: " << std::endl;
		c.each([&c](const EntityID& id, Entity& e) -> void {
			std::cout << "\t" << id.name() << std::endl;
		});
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

/// ======================== the main function ========================
int main(int, char*[]) {

	auto cube = std::make_unique<Entity>();
	cube->setComponent("Position", std::make_unique<PositionCmp>());
	cube->setComponent("Velocity", std::make_unique<VelocityCmp>());

	auto circle = std::make_unique<Entity>();
	circle->setComponent("Position", std::make_unique<PositionCmp>());
	circle->setComponent("Velocity", std::make_unique<VelocityCmp>());

	auto foo = std::make_unique<Entity>();
	foo->setComponent("Position", std::make_unique<PositionCmp>());
	foo->setComponent("Velocity", std::make_unique<PositionCmp>());
	foo->setComponent("Mesh", std::make_unique<PositionCmp>());


	std::cout << std::endl << "------------------------------------------------------------------" << std::endl;

	using one = std::integral_constant<int, 1>;
	using two = std::integral_constant<int, 2>;
	using five = std::integral_constant<int, two::value + two::value + one::value>;

	std::cout << typeid(one).name() << std::endl;
	std::cout << typeid(two).name() << std::endl;
	std::cout << typeid(five).name() << std::endl;

	std::cout << std::endl << "------------------------------------------------------------------" << std::endl;

	std::cout << "\t[cube]" << std::endl;
	std::cout << "Has Position: " << tf(cube->hasComponent<PositionCmp>("Position")) << std::endl;
	std::cout << "Has Velocity: " << tf(cube->hasComponent<VelocityCmp>("Velocity")) << std::endl;
	std::cout << "Has Mesh: " << (cube->hasComponent<MeshCmp>("Mesh") ? "true" : "false") << std::endl;
	std::cout << "Has Simulation Aspect: " << (simAspect.hasAspect(*cube) ? "true" : "false") << std::endl;
	std::cout << "Has Drawing Aspect: " << (drawAspect.hasAspect(*cube) ? "true" : "false") << std::endl;

	std::cout << "\t[circle]" << std::endl;
	std::cout << "Has Position: " << (circle->hasComponent<PositionCmp>("Position") ? "true" : "false") << std::endl;
	std::cout << "Has Velocity: " << (circle->hasComponent<VelocityCmp>("Velocity") ? "true" : "false") << std::endl;
	std::cout << "Has Mesh: " << (circle->hasComponent<MeshCmp>("Mesh") ? "true" : "false") << std::endl;
	std::cout << "Has Simulation Aspect: " << (simAspect.hasAspect(*circle) ? "true" : "false") << std::endl;
	std::cout << "Has Drawing Aspect: " << (drawAspect.hasAspect(*circle) ? "true" : "false") << std::endl;
	
	Context world;
	world.addSystem<DrawingSystem>();
	world.addSystem<SimulationSystem>();
	world.addSystem<UpdaterSystem>();

	world.setEntity("cube", std::move(cube));
	world.setEntity("circle", std::move(circle));
	world.setEntity("foo", std::move(foo));

	std::cout << std::endl << "------------------------------------------------------------------" << std::endl;
	std::cout << "Updating the world.." << std::endl;
	world.updateSystems();
	world.updateSystems();

	std::cout << std::endl << "------------------------------------------------------------------" << std::endl;

	testSizeof<char*>();
	testSizeof<std::unique_ptr<char>>();
	testSizeof<std::shared_ptr<char>>();
	testSizeof<uint64_t>();
	testSizeof<size_t>();
	testSizeof<int>();

	std::cout << std::endl << "------------------------------------------------------------------" << std::endl;
	// testing the identifier function
	std::array<char, sizeof("hello world!")> s{"hello world!" };
	std::array<char, sizeof("blabla")> S = { "blabla" };
	//std::array<char, sizeof("hello world! ")> s2{ s,'h' };

	CONSTEXPR Identifier i( "blabla" );
	CONSTEXPR Identifier j("blablu");
	CONSTEXPR Identifier k("ABCDEFGHIJKLMNOPQRSTUVWXYZ1234");
	CONSTEXPR Identifier l("ABCDEFGHIJKLMNOPQRSTUVWXYZ12345");
	CONSTEXPR Identifier m("ABCDEFGHIJKLMNOPQRSTUVWXYZ123456");
	CONSTEXPR Identifier n("ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567");
	CONSTEXPR bool ieqj = i == j;
	//constexpr uint64_t ihash{ i.hash() }; // somehow this does not run in VS, might be compiler bug
	CONSTEXPR bool iltj = i < j;

	std::cout << "Identifier i: " << i.name() << " " << i.hash() << std::endl;
	std::cout << "Identifier j: " << j.name() << " " << j.hash() << std::endl;
	std::cout << "Identifier k: " << k.name() << " " << k.hash() << std::endl;
	std::cout << "Identifier l: " << l.name() << " " << l.hash() << std::endl;
	std::cout << "Identifier m: " << m.name() << " " << m.hash() << std::endl;
	std::cout << "Identifier n: " << n.name() << " " << n.hash() << std::endl;

	std::cout << "i == j: " << tf(ieqj) << std::endl;
	std::cout << "i == k: " << tf(i == k) << std::endl;
	std::cout << "k == l: " << tf(k == l) << std::endl;
	std::cout << "l == m: " << tf(l == m) << std::endl;
	std::cout << "m == n: " << tf(m == n) << std::endl;
	std::cout << "i < j: " << tf(iltj) << std::endl;

	// testing hash function
	std::unordered_map<Identifier, std::shared_ptr<Component>> map;
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
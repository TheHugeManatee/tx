#include "Entity.h"
#include "Component.h"
#include "System.h"
#include "Aspect.h"
#include "Context.h"
#include "Identifier.h"

#include <vector>

#include <iostream>

#include <typeindex>

struct Vec3 {
	double x, y, z;
};

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
	TagCmp(const std::string& tag_) : tag(tag_) {};
	const std::string tag;
};

using SimAspect = Aspect<PositionCmp, VelocityCmp>;
using DrawAspect = Aspect<PositionCmp, MeshCmp>;
using AllAspect = Aspect<PositionCmp, VelocityCmp, MeshCmp>;

class DrawingSystem : public AspectFilteringSystem<DrawAspect> {
public:
	void onEntityAdded(Context&, PositionCmp&, MeshCmp&) override {
		std::cout << "Drawing System noticed a new Entity!" << std::endl;
	}
};

class SimulationSystem : public AspectFilteringSystem<SimAspect> {
public:
	void onEntityAdded(Context&, PositionCmp&, VelocityCmp&) override {
		std::cout << "Simulation System noticed a new Entity!" << std::endl;
	}
};

class UpdaterSystem : public System {
public:
	void onEntityAdded(Context&, Entity& e) override {
		std::cout << "Updater System noticed a new Entity: " << e.stringify() << std::endl;
	}
};

template<typename T> void testSizeof() {
	std::cout << "Size of " << typeid(T).name() << ": " << sizeof(T) << std::endl;
}

template<typename T> void testSizeof(const T&) {
	std::cout << "Size of " << typeid(T).name() << ": " << sizeof(T) << std::endl;
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

int main(int, char*[]) {
	auto cube = std::make_shared<Entity>();
	cube->addComponent(std::make_shared<PositionCmp>());
	cube->addComponent(std::make_shared<VelocityCmp>());

	auto circle = std::make_shared<Entity>();
	circle->addComponent(std::make_shared<PositionCmp>());
	circle->addComponent(std::make_shared<MeshCmp>());

	auto foo = std::make_shared<Entity>();
	foo->ensureComponents<PositionCmp>();


	std::cout << std::endl << "------------------------------------------------------------------" << std::endl;

	using one = std::integral_constant<int, 1>;
	using two = std::integral_constant<int, 2>;
	using five = std::integral_constant<int, two::value + two::value + one::value>;

	std::cout << typeid(one).name() << std::endl;
	std::cout << typeid(two).name() << std::endl;
	std::cout << typeid(five).name() << std::endl;

	std::cout << std::endl << "------------------------------------------------------------------" << std::endl;

	std::cout << "\t[cube]" << std::endl;
	std::cout << "Has Position: " << tf(cube->hasComponent<PositionCmp>()) << std::endl;
	std::cout << "Has Velocity: " << cube->hasComponent<VelocityCmp>() << std::endl;
	std::cout << "Has Mesh: " << (cube->hasComponent<MeshCmp>() ? "true" : "false") << std::endl;
	std::cout << "Has Simulation Aspect: " << (SimAspect::hasAspect(*cube) ? "true" : "false") << std::endl;
	std::cout << "Has Drawing Aspect: " << (DrawAspect::hasAspect(*cube) ? "true" : "false") << std::endl;

	std::cout << "\t[circle]" << std::endl;
	std::cout << "Has Position: " << (circle->hasComponent<PositionCmp>() ? "true" : "false") << std::endl;
	std::cout << "Has Velocity: " << (circle->hasComponent<VelocityCmp>() ? "true" : "false") << std::endl;
	std::cout << "Has Mesh: " << (circle->hasComponent<MeshCmp>() ? "true" : "false") << std::endl;
	std::cout << "Has Simulation Aspect: " << (SimAspect::hasAspect(*circle) ? "true" : "false") << std::endl;
	std::cout << "Has Drawing Aspect: " << (DrawAspect::hasAspect(*circle) ? "true" : "false") << std::endl;

	Context world;
	world.addSystem<DrawingSystem>();
	world.addSystem<SimulationSystem>();
	world.addSystem<UpdaterSystem>();

	world.addEntity(cube);
	world.addEntity(circle);
	world.addEntity(foo);

	std::cout << std::endl << "------------------------------------------------------------------" << std::endl;

	testSizeof<char*>();
	testSizeof<std::unique_ptr<char>>();
	testSizeof<std::shared_ptr<char>>();
	testSizeof<uint64_t>();
	testSizeof<size_t>();
	testSizeof<int>();

	std::cout << std::endl << "------------------------------------------------------------------" << std::endl;

	std::array<char, sizeof("hello world!")> s{"hello world!" };
	std::array<char, sizeof("blabla")> S = { "blabla" };
	//std::array<char, sizeof("hello world! ")> s2{ s,'h' };

	constexpr Identifier i( "blabla" );
	constexpr Identifier j("blablu");
	constexpr Identifier k("ABCDEFGHIJKLMNOPQRSTUVWXYZ1234");
	constexpr Identifier l("ABCDEFGHIJKLMNOPQRSTUVWXYZ12345");
	constexpr Identifier m("ABCDEFGHIJKLMNOPQRSTUVWXYZ123456");
	constexpr Identifier n("ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567");
	constexpr bool ieqj = i == j;
	//constexpr uint64_t ihash{ i.hash() }; // somehow this does not run in VS, might be compiler bug
	constexpr bool iltj = i < j;

	std::cout << "Identifier " << i.name() << " " << i.hash() << std::endl;
	std::cout << "Identifier " << j.name() << " " << j.hash() << std::endl;
	std::cout << "Identifier " << k.name() << " " << k.hash() << std::endl;
	std::cout << "Identifier " << l.name() << " " << l.hash() << std::endl;
	std::cout << "Identifier " << m.name() << " " << m.hash() << std::endl;
	std::cout << "Identifier " << n.name() << " " << n.hash() << std::endl;

	std::cout << "i == j: " << tf(ieqj) << std::endl;
	std::cout << "i == k: " << tf(i == k) << std::endl;
	std::cout << "k == l: " << tf(k == l) << std::endl;
	std::cout << "l == m: " << tf(l == m) << std::endl;
	std::cout << "m == n: " << tf(m == n) << std::endl;
	std::cout << "i < j: " << tf(iltj) << std::endl;

	std::map<Identifier, std::shared_ptr<Component>> map;
	map[i] = std::make_shared<PositionCmp>();
	map[j] = std::make_shared<PositionCmp>();

	std::cout << "Hash of 's' is " << hashTest("s") << std::endl; // test implicit identifier instantiation

	std::cout << std::endl << "------------------------------------------------------------------" << std::endl;

	testSizeof(typeid(std::map<int, int>));
	testSizeof(std::type_index(typeid(std::map<int, int>)));
	testSizeof<ComponentID>();
	testSizeof<EntityID>();

	std::cout << "Strong typedef test: " << idName(Identifier("Identifier")) << ", " << idName(ComponentID("ComponentID")) << ", " << idName(EntityID("EntityID")) << std::endl;

	std::cout << std::endl << "------------------------------------------------------------------" << std::endl;
	std::cout << "Fin." << std::endl;
}
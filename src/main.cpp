#include "Entity.h"
#include "Component.h"
#include "System.h"
#include "Aspect.h"
#include "Context.h"

#include <vector>

#include <iostream>

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
	void onEntityAdded(PositionCmp&, MeshCmp&) override {
		std::cout << "Drawing System noticed a new Entity!" << std::endl;
	}
};

class SimulationSystem : public AspectFilteringSystem<SimAspect> {
public:
	void onEntityAdded(PositionCmp&, VelocityCmp&) override {
		std::cout << "Simulation System noticed a new Entity!" << std::endl;
	}
};

class UpdaterSystem : public System {
public:
	void onEntityAdded(Entity& e) override {
		std::cout << "Updater System noticed a new Entity: " << e.stringify() << std::endl;
	}
};

template<typename T> void testSizeof() {
	std::cout << "Size of " << typeid(T).name() << ": " << sizeof(T) << std::endl;
}

int main(int , char* [] ) {
	auto cube = std::make_shared<Entity>();
	cube->addComponent(std::make_shared<PositionCmp>());
	cube->addComponent(std::make_shared<VelocityCmp>());

	auto circle = std::make_shared<Entity>();
	circle->addComponent(std::make_shared<PositionCmp>());
	circle->addComponent(std::make_shared<MeshCmp>());

	auto foo = std::make_shared<Entity>();
	foo->ensureComponents<PositionCmp>();

	using one = std::integral_constant<int, 1>;
	using two = std::integral_constant<int, 2>;
	using five = std::integral_constant<int, two::value + two::value + one::value>;

	std::cout << typeid(one).name() << std::endl;
	std::cout << typeid(two).name() << std::endl;
	std::cout << typeid(five).name() << std::endl;

	

	std::cout << "\t[cube]" << std::endl;
	std::cout << "Has Position: " << (cube->hasComponent<PositionCmp>() ? "true" : "false") << std::endl;
	std::cout << "Has Velocity: " << (cube->hasComponent<VelocityCmp>() ? "true" : "false") << std::endl;
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


	testSizeof<char*>();
	testSizeof<std::unique_ptr<char>>();
	testSizeof<std::shared_ptr<char>>();
	testSizeof<uint64_t>();
	testSizeof<size_t>();
	testSizeof<int>();
}
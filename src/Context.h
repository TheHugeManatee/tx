#pragma once

#include "Entity.h"
#include "System.h"

#include <vector>

class Context {
public:
	Context() {};

	~Context() {};

	void addEntity(std::shared_ptr<Entity> entity) {
		entities_.push_back(entity);

		for (auto& sys : systems_) {
			if (sys->isInterested(*entity)) {
				sys->onEntityAdded(*entity);
			}
		}
	}

	template<class S, typename... Args>
	void addSystem(Args... args) {
		// TODO check if already in there
		systems_.push_back(std::make_unique<S>(std::forward<Args>(args)...));
	}

private:
	std::vector<std::shared_ptr<Entity>> entities_;
	std::vector<std::unique_ptr<System>> systems_;
};


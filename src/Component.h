#pragma once

#include <typeinfo>

class Component {
public:
	Component() {};
	virtual ~Component() {};

	//Component& operator=(Component&& cmp) = default;

	size_t hash() const NOEXCEPT {
		return typeid(*this).hash_code();
	}

private:

};

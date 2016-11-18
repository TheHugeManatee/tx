
#include <unordered_map>
#include <iostream>

#include "Entity.h"

int main(int argc, char** argv) {

    std::unordered_map<size_t, Entity> emap;

    for (const auto& e : emap) {
        std::cout << e.first << std::endl;
    }

}
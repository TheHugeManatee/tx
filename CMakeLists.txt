CMAKE_MINIMUM_REQUIRED(VERSION 3.0.2 FATAL_ERROR)

project(ECS)

FILE(GLOB ECSHeaders
    src/*.h
)
FILE(GLOB ECSSource
    src/*.cpp
)

# ADD_DEFINITIONS()
INCLUDE_DIRECTORIES(src)
# TARGET_LINK_LIBRARIES()

add_executable(ecs ${ECSSource} ${ECSHeaders})


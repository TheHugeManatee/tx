CMAKE_MINIMUM_REQUIRED(VERSION 3.0.2 FATAL_ERROR)

project(ECS)

FILE(GLOB ECSHeaders
    src/*.h
)
FILE(GLOB ECSSource
    src/*.cpp
)

IF(MSVC12)
	add_definitions(-DOVERRIDE=)
	add_definitions(-DNOEXCEPT=)
	add_definitions(-DCONSTEXPR=)
ELSE()
	add_definitions(-DOVERRIDE=override)
	add_definitions(-DNOEXCEPT=noexcept)
	add_definitions(-DCONSTEXPR=constexpr)
ENDIF()

# ADD_DEFINITIONS()
INCLUDE_DIRECTORIES(src)
# TARGET_LINK_LIBRARIES()

add_executable(ecs ${ECSSource} ${ECSHeaders})


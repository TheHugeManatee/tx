CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12 FATAL_ERROR)

project(TX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

FILE(GLOB TXHeaders
    src/*.h
)
FILE(GLOB TXSource
    src/main.cpp
)

IF(MSVC12)
	add_definitions(-DOVERRIDE=override)
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

add_executable(ecs-example ${TXSource} ${TXHeaders})

include_directories(src)
add_executable(testing src/testing.cpp)



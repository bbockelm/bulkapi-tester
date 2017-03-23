
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(BulkApi-Tester)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

#add_definitions(-std=c++14)
add_definitions(-O3)
add_definitions(-g)
add_definitions(-march=native)

set (CMAKE_EXE_LINKER_FLAGS "-flto -fwhole-program -O3")

find_package(ROOT REQUIRED COMPONENTS RIO TreePlayer)
include(${ROOT_USE_FILE})

add_subdirectory(src)


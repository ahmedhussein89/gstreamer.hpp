# ${CMAKE_SOURCE_DIR}/CMakeLists.txt
cmake_minimum_required(VERSION 3.22)
project(deepstream.hpp
        VERSION 0.1.0
        DESCRIPTION "A Modern C++ Wrapper for NVIDIA DeepStream"
        HOMEPAGE_URL "https://github.com/ahmedhussein89/deepstream.hpp"
        LANGUAGES C CXX)

option(DS_BUILD_EXAMPLES "" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

find_package(DeepStream 7.1 REQUIRED)

if(DS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

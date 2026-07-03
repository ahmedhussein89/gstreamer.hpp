# ${CMAKE_SOURCE_DIR}/CMakeLists.txt
cmake_minimum_required(VERSION 3.22)
project(deepstream.hpp
        VERSION 0.1.0
        DESCRIPTION "A Modern C++ Wrapper for NVIDIA DeepStream"
        HOMEPAGE_URL "https://github.com/ahmedhussein89/deepstream.hpp"
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(DS_BUILD_EXAMPLES "Build DeepStream examples" OFF)
option(DS_BUILD_TUTORIALS "Build all tutorials" ON)
option(DS_USE_EXPECTED_LITE "Use expected-lite library" ON)
option(DS_BUILD_TESTS "Use tests" ON)
option(DS_ENABLE_SANITIZERS "Enable sanitizers for all targets" OFF)
set(DS_SANITIZER "address" CACHE STRING "Sanitizer to use: address, memory, thread, undefined, or none")
set_property(CACHE DS_SANITIZER PROPERTY STRINGS "address" "memory" "thread" "undefined" "none")
option(DS_ENABLE_CLANG_TIDY "Enable clang-tidy static analysis during build" OFF)

include(${CMAKE_SOURCE_DIR}/cmake/compiler_warnings.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/coverage.cmake)
if(DS_ENABLE_CLANG_TIDY)
  include(${CMAKE_SOURCE_DIR}/cmake/clang_tidy.cmake)
endif()
if(DS_ENABLE_SANITIZERS)
  include(${CMAKE_SOURCE_DIR}/cmake/sanitizers.cmake)
else()
  set(SELECTED_SANITIZER "")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

find_package(DeepStream QUIET)
if(DeepStream_FOUND)
  message(STATUS "DeepStream found: ${DeepStream_ROOT_DIR} (${DeepStream_VERSION}) -- metadata module enabled")
else()
  message(STATUS "DeepStream not found -- metadata module will not be built")
endif()

find_package(expected-lite REQUIRED)
find_package(fmt REQUIRED)
find_package(GStreamer REQUIRED COMPONENTS Video)
find_package(spdlog REQUIRED)

add_subdirectory(include)

if(DS_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(DS_BUILD_TUTORIALS)
  add_subdirectory(tutorials)
endif()

if(DS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

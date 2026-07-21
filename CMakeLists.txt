# ${CMAKE_SOURCE_DIR}/CMakeLists.txt
cmake_minimum_required(VERSION 3.22)
project(gstreamer.hpp
        VERSION 0.1.0
        DESCRIPTION "A Modern C++ Wrapper for GStreamer"
        HOMEPAGE_URL "https://github.com/ahmedhussein89/gstreamer.hpp"
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(GST_BUILD_TUTORIALS "Build all tutorials" ON)
option(GST_USE_EXPECTED_LITE "Use expected-lite library" ON)
option(GST_BUILD_TESTS "Use tests" ON)
option(GST_ENABLE_SANITIZERS "Enable sanitizers for all targets" OFF)
set(GST_SANITIZER "address" CACHE STRING "Sanitizer to use: address, memory, thread, undefined, or none")
set_property(CACHE GST_SANITIZER PROPERTY STRINGS "address" "memory" "thread" "undefined" "none")
option(GST_ENABLE_CLANG_TIDY "Enable clang-tidy static analysis during build" OFF)

include(${CMAKE_SOURCE_DIR}/cmake/compiler_warnings.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/coverage.cmake)
if(GST_ENABLE_CLANG_TIDY)
  include(${CMAKE_SOURCE_DIR}/cmake/clang_tidy.cmake)
endif()
if(GST_ENABLE_SANITIZERS)
  include(${CMAKE_SOURCE_DIR}/cmake/sanitizers.cmake)
else()
  set(SELECTED_SANITIZER "")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

find_package(expected-lite REQUIRED)
find_package(fmt REQUIRED)
find_package(GStreamer REQUIRED COMPONENTS Video)

add_subdirectory(include)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(GST_BUILD_TUTORIALS)
  add_subdirectory(tutorials)
endif()

if(GST_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# ${CMAKE_SOURCE_DIR}/CMakeLists.txt
cmake_minimum_required(VERSION 3.22)
project(deepstream.hpp
        VERSION 0.1.0
        DESCRIPTION "A Modern C++ Wrapper for NVIDIA DeepStream"
        HOMEPAGE_URL "https://github.com/ahmedhussein89/deepstream.hpp"
        LANGUAGES C CXX)

option(DS_BUILD_EXAMPLES "Build DeepStream examples" OFF)
option(DS_BUILD_TUTORIALS "Build all tutorials" ON)
option(DS_USE_EXPECTED_LITE "Use expected-lite library" ON)
option(DS_ENABLE_SANITIZERS "Enable sanitizers for all targets" OFF)
set(DS_SANITIZER "address" CACHE STRING "Sanitizer to use: address, memory, thread, undefined, or none")
set_property(CACHE DS_SANITIZER PROPERTY STRINGS "address" "memory" "thread" "undefined" "none")

include(${CMAKE_SOURCE_DIR}/cmake/compiler_warnings.cmake)
if(DS_ENABLE_SANITIZERS)
  include(${CMAKE_SOURCE_DIR}/cmake/sanitizers.cmake)
else()
  set(SELECTED_SANITIZER "")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

# External dependencies
include(FetchContent)

if(DS_USE_EXPECTED_LITE)
  FetchContent_Declare(expected-lite
    GIT_REPOSITORY https://github.com/nonstd-lite/expected-lite.git
    GIT_TAG master
  )
  FetchContent_MakeAvailable(expected-lite)
endif()

# fmt - shipped with the library
FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG master
)
FetchContent_MakeAvailable(fmt)

# spdlog - for tutorials and examples only
if(DS_BUILD_EXAMPLES OR DS_BUILD_TUTORIALS)
  FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG master
  )
  FetchContent_MakeAvailable(spdlog)
endif()

# find_package(DeepStream 7.1 REQUIRED)
find_package(GStreamer REQUIRED COMPONENTS Video)

if(DS_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(DS_BUILD_TUTORIALS)
  add_subdirectory(tutorials)
endif()

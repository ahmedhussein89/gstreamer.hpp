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
option(GST_ENABLE_CPPCHECK "Enable cppcheck static analysis during build" OFF)

include(${CMAKE_SOURCE_DIR}/cmake/compiler_warnings.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/coverage.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/linker.cmake)
if(GST_ENABLE_CLANG_TIDY)
  include(${CMAKE_SOURCE_DIR}/cmake/clang_tidy.cmake)
endif()
if(GST_ENABLE_CPPCHECK)
  include(${CMAKE_SOURCE_DIR}/cmake/cppcheck.cmake)
endif()
if(GST_ENABLE_SANITIZERS)
  include(${CMAKE_SOURCE_DIR}/cmake/sanitizers.cmake)
else()
  set(SELECTED_SANITIZER "")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

# Package managers (vcpkg, conan) supply fmt/expected-lite themselves; set this ON there.
# Off by default so a plain repo build keeps using the pinned submodules.
option(GST_USE_SYSTEM_DEPS "Consume fmt/expected-lite via find_package instead of the vendored submodules" OFF)

if(GST_USE_SYSTEM_DEPS)
  find_package(expected-lite REQUIRED)
  find_package(fmt REQUIRED)
else()
  set(GST_EXPECTED_LITE_VENDORED ON)
  add_subdirectory(third_party/expected-lite)
  set(GST_FMT_VENDORED ON)
  # fmt only installs itself when it is the master project; we need its config for our consumers.
  set(FMT_INSTALL ON CACHE BOOL "" FORCE)
  add_subdirectory(third_party/fmt)
endif()
if(GST_BUILD_TUTORIALS)
  set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
  add_subdirectory(third_party/tracy)
endif()

# Static analyzers must never run over vendored third_party sources.
if(GST_ENABLE_CLANG_TIDY OR GST_ENABLE_CPPCHECK)
  if(GST_EXPECTED_LITE_VENDORED)
    get_property(_gst_tp_targets DIRECTORY third_party/expected-lite PROPERTY BUILDSYSTEM_TARGETS)
    list(APPEND _gst_tp_targets_all ${_gst_tp_targets})
  endif()
  if(GST_FMT_VENDORED)
    get_property(_gst_tp_targets DIRECTORY third_party/fmt PROPERTY BUILDSYSTEM_TARGETS)
    list(APPEND _gst_tp_targets_all ${_gst_tp_targets})
  endif()
  if(GST_BUILD_TUTORIALS)
    get_property(_gst_tp_targets DIRECTORY third_party/tracy PROPERTY BUILDSYSTEM_TARGETS)
    list(APPEND _gst_tp_targets_all ${_gst_tp_targets})
  endif()
  foreach(_gst_tp_target ${_gst_tp_targets_all})
    set_target_properties(${_gst_tp_target} PROPERTIES CXX_CLANG_TIDY "" C_CLANG_TIDY ""
                                                         CXX_CPPCHECK "" C_CPPCHECK "")
  endforeach()
endif()
find_package(GStreamer REQUIRED COMPONENTS Video
             OPTIONAL_COMPONENTS Base Net RtspServer Check Pbutils)

add_subdirectory(include)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# enable_testing() must precede any add_subdirectory() containing add_test(),
# otherwise those directories are processed with testing disabled and their
# tests never reach CTestTestfile.cmake (tutorials/hard/TestingElements).
if(GST_BUILD_TESTS)
  enable_testing()
endif()

if(GST_BUILD_TUTORIALS)
  add_subdirectory(tutorials)
endif()

if(GST_BUILD_TESTS)
  add_subdirectory(tests)
endif()

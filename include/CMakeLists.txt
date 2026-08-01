# ${CMAKE_SOURCE_DIR}/include/CMakeLists.txt
add_library(gstreamer_hpp INTERFACE)

add_library(gstreamer::hpp ALIAS gstreamer_hpp)

target_sources(
    gstreamer_hpp
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gstreamer.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gstreamer_raii.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core/core.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core/handle.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core/flags.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core/enums.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core/array_proxy.hpp>)

target_include_directories(
    gstreamer_hpp
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(
    gstreamer_hpp
    INTERFACE
    fmt::fmt
    nonstd::expected-lite
    GStreamer::GStreamer)

# Warnings are a build-time concern of this repo, never of a consumer of the installed package.
target_link_libraries(gstreamer_hpp INTERFACE $<BUILD_INTERFACE:gstreamer::warnings>)

# ── Install ────────────────────────────────────────────────────────────────────
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Only our own target is exported; vendored fmt/expected-lite install their own CMake configs,
# which gstreamer-hpp-config.cmake picks up via find_dependency().
set_target_properties(gstreamer_hpp PROPERTIES EXPORT_NAME hpp)

install(TARGETS gstreamer_hpp EXPORT gstreamer-hpp-targets)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.hpp"
    PATTERN "CMakeLists.txt" EXCLUDE)

set(_gst_cmake_dir ${CMAKE_INSTALL_LIBDIR}/cmake/gstreamer-hpp)

install(
    EXPORT gstreamer-hpp-targets
    FILE gstreamer-hpp-targets.cmake
    NAMESPACE gstreamer::
    DESTINATION ${_gst_cmake_dir})

configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/gstreamer-hpp-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/gstreamer-hpp-config.cmake
    INSTALL_DESTINATION ${_gst_cmake_dir})

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/gstreamer-hpp-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT)

install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/gstreamer-hpp-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/gstreamer-hpp-config-version.cmake
    ${CMAKE_SOURCE_DIR}/cmake/Modules/FindGStreamer.cmake
    DESTINATION ${_gst_cmake_dir})

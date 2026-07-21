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

target_link_libraries(gstreamer_hpp INTERFACE gstreamer::warnings)

# ── Install ────────────────────────────────────────────────────────────────────
include(GNUInstallDirs)

install(
    TARGETS
    gstreamer_warnings
    gstreamer_warnings_strict
    gstreamer_hpp
    fmt
    expected-lite
    EXPORT gstreamer-hpp-targets)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.hpp"
    PATTERN "CMakeLists.txt" EXCLUDE)

install(
    EXPORT gstreamer-hpp-targets
    FILE gstreamer-hpp-targets.cmake
    NAMESPACE gstreamer::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/gstreamer-hpp)

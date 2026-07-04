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

target_link_libraries(gstreamer_hpp INTERFACE deepstream::warnings)


add_library(deepstream_hpp INTERFACE)

add_library(ds::hpp ALIAS deepstream_hpp)

target_sources(
    deepstream_hpp
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/deepstream.hpp>)

target_include_directories(
    deepstream_hpp
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

if(DeepStream_FOUND)
  target_compile_definitions(deepstream_hpp INTERFACE DS_HAS_DEEPSTREAM)
  target_link_libraries(deepstream_hpp INTERFACE deepstream_metadata)
endif()

target_link_libraries(deepstream_hpp INTERFACE deepstream::warnings)

add_library(deepstream_raii_hpp INTERFACE)

add_library(ds::raii ALIAS deepstream_raii_hpp)

target_sources(
    deepstream_raii_hpp
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/deepstream_raii.hpp>)

target_include_directories(
    deepstream_raii_hpp
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(
    deepstream_raii_hpp
    INTERFACE
    deepstream_hpp
    deepstream_elements)

target_link_libraries(deepstream_raii_hpp INTERFACE deepstream::warnings)

add_library(deepstream_elements INTERFACE)

add_library(ds::elements ALIAS deepstream_elements)

target_sources(
    deepstream_elements
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/elements.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/builder.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils/error.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils/debug.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/elements/sources.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/elements/transformations.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/elements/inference.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/elements/tracking.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/elements/sinks.hpp>)

target_include_directories(
    deepstream_elements
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(
    deepstream_elements
    INTERFACE
    gstreamer_hpp
    fmt::fmt
    spdlog::spdlog)

target_link_libraries(deepstream_elements INTERFACE deepstream::warnings)

if(DeepStream_FOUND)
  add_library(deepstream_metadata INTERFACE)
  add_library(ds::metadata ALIAS deepstream_metadata)

  target_sources(
      deepstream_metadata
      INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/metadata.hpp>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/metadata/meta_list_view.hpp>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/metadata/batch_meta.hpp>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/metadata/frame_meta.hpp>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/metadata/object_meta.hpp>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/metadata/classifier_meta.hpp>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/metadata/tensor_meta.hpp>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/metadata/user_meta.hpp>)

  target_include_directories(
      deepstream_metadata
      INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

  target_link_libraries(
      deepstream_metadata
      INTERFACE
      gstreamer_hpp
      DeepStream::nvdsgst_meta
      DeepStream::nvds_meta)

  target_link_libraries(deepstream_metadata INTERFACE deepstream::warnings)
endif()

# ── Install ────────────────────────────────────────────────────────────────────
include(GNUInstallDirs)

set(_ds_install_targets
    deepstream_warnings
    deepstream_warnings_strict
    gstreamer_hpp
    deepstream_hpp
    deepstream_raii_hpp
    deepstream_elements)
if(DeepStream_FOUND)
  list(APPEND _ds_install_targets deepstream_metadata)
endif()

install(
    TARGETS ${_ds_install_targets}
    EXPORT deepstream-hpp-targets)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.hpp"
    PATTERN "CMakeLists.txt" EXCLUDE)

install(
    EXPORT deepstream-hpp-targets
    FILE deepstream-hpp-targets.cmake
    NAMESPACE ds::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deepstream-hpp)

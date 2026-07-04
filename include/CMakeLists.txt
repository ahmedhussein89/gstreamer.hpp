# ${CMAKE_SOURCE_DIR}/include/CMakeLists.txt
add_library(gstreamer_hpp INTERFACE)

add_library(gstreamer::hpp ALIAS gstreamer_hpp)

target_sources(
    gstreamer_hpp
    INTERFACE
    gstreamer.hpp
    gstreamer_raii.hpp
    core/core.hpp
    core/handle.hpp
    core/flags.hpp
    core/enums.hpp
    core/array_proxy.hpp)

target_include_directories(
    gstreamer_hpp
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR})

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
    deepstream.hpp)

target_include_directories(
    deepstream_hpp
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR})

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
    deepstream_raii.hpp)

target_include_directories(
    deepstream_raii_hpp
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR})

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
    elements.hpp
    builder.hpp
    utils/error.hpp
    utils/debug.hpp
    elements/sources.hpp
    elements/transformations.hpp
    elements/inference.hpp
    elements/tracking.hpp
    elements/sinks.hpp)

target_include_directories(
    deepstream_elements
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR})

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
      metadata.hpp
      metadata/meta_list_view.hpp
      metadata/batch_meta.hpp
      metadata/frame_meta.hpp
      metadata/object_meta.hpp
      metadata/classifier_meta.hpp
      metadata/tensor_meta.hpp
      metadata/user_meta.hpp)

  target_include_directories(
      deepstream_metadata
      INTERFACE
      ${CMAKE_CURRENT_SOURCE_DIR})

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

set(_ds_install_targets gstreamer_hpp deepstream_hpp deepstream_raii_hpp deepstream_elements)
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

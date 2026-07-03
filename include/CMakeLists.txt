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

myproject_set_project_warnings(gstreamer_hpp TRUE "" "" "" "")

add_library(pipeline_hpp INTERFACE)

add_library(pipeline::hpp ALIAS pipeline_hpp)

target_sources(
    pipeline_hpp
    INTERFACE
    pipeline.hpp)

target_include_directories(
    pipeline_hpp
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
    pipeline_hpp
    INTERFACE
    gstreamer_hpp
    fmt::fmt
    nonstd::expected-lite)

myproject_set_project_warnings(pipeline_hpp TRUE "" "" "" "")

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

myproject_set_project_warnings(deepstream_elements TRUE "" "" "" "")

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

  myproject_set_project_warnings(deepstream_metadata TRUE "" "" "" "")
endif()

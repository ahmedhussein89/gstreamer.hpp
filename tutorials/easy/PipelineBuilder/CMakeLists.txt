# ${CMAKE_SOURCE_DIR}/tutorials/easy/PipelineBuilder/CMakeLists.txt
add_executable(PipelineBuilder)

target_sources(
    PipelineBuilder
    PRIVATE
    main.cpp)

target_link_libraries(
    PipelineBuilder
    PRIVATE
    fmt::fmt
    GStreamer::GStreamer
    ${SELECTED_SANITIZER})

target_link_libraries(PipelineBuilder PRIVATE deepstream::warnings)

add_executable(PipelineBuilderView)

target_sources(
    PipelineBuilderView
    PRIVATE
    main_view.cpp)

target_link_libraries(
    PipelineBuilderView
    PRIVATE
    fmt::fmt
    nonstd::expected-lite
    gstreamer::hpp
    GStreamer::GStreamer
    ${SELECTED_SANITIZER})

target_link_libraries(PipelineBuilderView PRIVATE deepstream::warnings)

add_executable(PipelineBuilderRAII)

target_sources(
    PipelineBuilderRAII
    PRIVATE
    main_raii.cpp)

target_link_libraries(
    PipelineBuilderRAII
    PRIVATE
    fmt::fmt
    nonstd::expected-lite
    gstreamer::hpp
    GStreamer::GStreamer
    ${SELECTED_SANITIZER})

target_link_libraries(PipelineBuilderRAII PRIVATE deepstream::warnings)

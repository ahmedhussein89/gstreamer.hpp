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

myproject_set_project_warnings(PipelineBuilder FALSE "" "" "" "")

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

myproject_set_project_warnings(PipelineBuilderView FALSE "" "" "" "")

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

myproject_set_project_warnings(PipelineBuilderRAII FALSE "" "" "" "")

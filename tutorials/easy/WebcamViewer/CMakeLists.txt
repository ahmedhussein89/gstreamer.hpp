# ${CMAKE_SOURCE_DIR}/tutorials/easy/WebcamViewer/CMakeLists.txt
add_executable(WebcamViewer)

target_sources(
    WebcamViewer
    PRIVATE
    main.cpp)

target_link_libraries(
    WebcamViewer
    PRIVATE
    fmt::fmt
    GStreamer::GStreamer
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(WebcamViewer FALSE "" "" "" "")

add_executable(WebcamViewerView)

target_sources(
    WebcamViewerView
    PRIVATE
    main_view.cpp)

target_link_libraries(
    WebcamViewerView
    PRIVATE
    fmt::fmt
    nonstd::expected-lite
    gstreamer::hpp
    GStreamer::GStreamer
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(WebcamViewerView FALSE "" "" "" "")

add_executable(WebcamViewerRAII)

target_sources(
    WebcamViewerRAII
    PRIVATE
    main_raii.cpp)

target_link_libraries(
    WebcamViewerRAII
    PRIVATE
    fmt::fmt
    nonstd::expected-lite
    gstreamer::hpp
    GStreamer::GStreamer
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(WebcamViewerRAII FALSE "" "" "" "")

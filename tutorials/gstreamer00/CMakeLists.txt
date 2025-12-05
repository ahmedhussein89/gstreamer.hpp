# ${CMAKE_SOURCE_DIR}/tutorials/gstreamer00/CMakeLists.txt
add_executable(gstreamer00)

target_sources(
    gstreamer00
    PRIVATE
    gstreamer00.c)

target_link_libraries(
    gstreamer00
    PRIVATE
    GStreamer::GStreamer
    GStreamer::Video)

add_executable(gstreamer00_cpp)

target_sources(
    gstreamer00_cpp
    PRIVATE
    gstreamer00.cpp)

target_link_libraries(
    gstreamer00_cpp
    PRIVATE
    fmt::fmt
    nonstd::expected-lite
    GStreamer::GStreamer
    GStreamer::Video
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(gstreamer00_cpp TRUE "" "" "" "")

# ${CMAKE_SOURCE_DIR}/tutorials/gstreamer01/CMakeLists.txt
add_executable(gstreamer01)

target_sources(
    gstreamer01
    PRIVATE
    gstreamer01.c)

target_link_libraries(
    gstreamer01
    PRIVATE
    GStreamer::GStreamer
    GStreamer::Video)

add_executable(gstreamer01_cpp)

target_sources(
    gstreamer01_cpp
    PRIVATE
    gstreamer01.cpp)

target_link_libraries(
    gstreamer01_cpp
    PRIVATE
    fmt::fmt
    nonstd::expected-lite
    gstreamer::hpp
    GStreamer::Video
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(gstreamer01_cpp TRUE "" "" "" "")

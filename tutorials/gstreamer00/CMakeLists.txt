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

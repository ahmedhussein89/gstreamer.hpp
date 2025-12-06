# ${CMAKE_SOURCE_DIR}/include/CMakeLists.txt
add_library(gstreamer_hpp INTERFACE)

add_library(gstreamer::hpp ALIAS gstreamer_hpp)

target_sources(
    gstreamer_hpp
    INTERFACE
    gstreamer.hpp)

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

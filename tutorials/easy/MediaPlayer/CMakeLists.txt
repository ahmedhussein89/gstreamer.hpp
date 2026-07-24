# ${CMAKE_SOURCE_DIR}/tutorials/easy/MediaPlayer/CMakeLists.txt
add_executable(MediaPlayer)

target_sources(
    MediaPlayer
    PRIVATE
    main_dynamic.cpp)

target_link_libraries(
    MediaPlayer
    PRIVATE
    fmt::fmt
    nonstd::expected-lite
    gstreamer::hpp
    GStreamer::Video
    ${SELECTED_SANITIZER})

target_link_libraries(MediaPlayer PRIVATE gstreamer::warnings)

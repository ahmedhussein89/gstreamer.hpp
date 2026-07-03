# ${CMAKE_SOURCE_DIR}/tutorials/easy/AudioPlayer/CMakeLists.txt
add_executable(AudioPlayer)

target_sources(
    AudioPlayer
    PRIVATE
    main.cpp)

target_link_libraries(
    AudioPlayer
    PRIVATE
    fmt::fmt
    nonstd::expected-lite
    gstreamer::hpp
    GStreamer::Video
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(AudioPlayer FALSE "" "" "" "")

add_executable(AudioPlayerView)

target_sources(
    AudioPlayerView
    PRIVATE
    main_view.cpp)

target_link_libraries(
    AudioPlayerView
    PRIVATE
    fmt::fmt
    nonstd::expected-lite
    gstreamer::hpp
    GStreamer::Video
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(AudioPlayerView FALSE "" "" "" "")

add_executable(AudioPlayerRAII)

target_sources(
    AudioPlayerRAII
    PRIVATE
    main_raii.cpp)

target_link_libraries(
    AudioPlayerRAII
    PRIVATE
    fmt::fmt
    nonstd::expected-lite
    gstreamer::hpp
    GStreamer::Video
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(AudioPlayerRAII FALSE "" "" "" "")

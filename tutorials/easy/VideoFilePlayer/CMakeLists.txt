# ${CMAKE_SOURCE_DIR}/tutorials/easy/VideoFilePlayer/CMakeLists.txt
add_executable(VideoFilePlayer)

target_sources(
    VideoFilePlayer
    PRIVATE
    main.cpp)

target_link_libraries(
    VideoFilePlayer
    PRIVATE
    fmt::fmt
    nonstd::expected-lite
    gstreamer::hpp
    GStreamer::Video
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(VideoFilePlayer FALSE "" "" "" "")

add_executable(VideoFilePlayerView)

target_sources(
    VideoFilePlayerView
    PRIVATE
    main_view.cpp)

target_link_libraries(
    VideoFilePlayerView
    PRIVATE
    fmt::fmt
    nonstd::expected-lite
    gstreamer::hpp
    GStreamer::Video
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(VideoFilePlayerView FALSE "" "" "" "")

add_executable(VideoFilePlayerRAII)

target_sources(
    VideoFilePlayerRAII
    PRIVATE
    main_raii.cpp)

target_link_libraries(
    VideoFilePlayerRAII
    PRIVATE
    fmt::fmt
    nonstd::expected-lite
    gstreamer::hpp
    GStreamer::Video
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(VideoFilePlayerRAII FALSE "" "" "" "")

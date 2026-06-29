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

add_executable(VideoFilePlayerDeclarative)

target_sources(
    VideoFilePlayerDeclarative
    PRIVATE
    main_declarative.cpp)

target_link_libraries(
    VideoFilePlayerDeclarative
    PRIVATE
    pipeline::hpp
    GStreamer::Video
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(VideoFilePlayerDeclarative FALSE "" "" "" "")

add_executable(VideoFilePlayerDynamic)

target_sources(
    VideoFilePlayerDynamic
    PRIVATE
    main_dynamic.cpp)

target_link_libraries(
    VideoFilePlayerDynamic
    PRIVATE
    fmt::fmt
    nonstd::expected-lite
    gstreamer::hpp
    GStreamer::Video
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(VideoFilePlayerDynamic FALSE "" "" "" "")
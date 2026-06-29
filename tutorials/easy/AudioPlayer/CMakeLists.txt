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

add_executable(AudioPlayerDeclarative)

target_sources(
    AudioPlayerDeclarative
    PRIVATE
    main_declarative.cpp)

target_link_libraries(
    AudioPlayerDeclarative
    PRIVATE
    pipeline::hpp
    GStreamer::Video
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(AudioPlayerDeclarative FALSE "" "" "" "")

add_executable(AudioPlayerDynamic)

target_sources(
    AudioPlayerDynamic
    PRIVATE
    main_dynamic.cpp)

target_link_libraries(
    AudioPlayerDynamic
    PRIVATE
    fmt::fmt
    nonstd::expected-lite
    gstreamer::hpp
    GStreamer::Video
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(AudioPlayerDynamic FALSE "" "" "" "")

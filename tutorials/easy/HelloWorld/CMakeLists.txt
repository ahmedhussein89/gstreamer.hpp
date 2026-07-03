# ${CMAKE_SOURCE_DIR}/tutorials/easy/HelloWorld/CMakeLists.txt
add_executable(HelloWorld)

target_sources(
    HelloWorld
    PRIVATE
    main.cpp)

target_link_libraries(
    HelloWorld
    PRIVATE
    fmt::fmt
    nonstd::expected-lite
    gstreamer::hpp
    GStreamer::Video
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(HelloWorld FALSE "" "" "" "")

add_executable(HelloWorldView)

target_sources(
    HelloWorldView
    PRIVATE
    main_view.cpp)

target_link_libraries(
    HelloWorldView
    PRIVATE
    fmt::fmt
    nonstd::expected-lite
    gstreamer::hpp
    GStreamer::Video
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(HelloWorldView FALSE "" "" "" "")

add_executable(HelloWorldRAII)

target_sources(
    HelloWorldRAII
    PRIVATE
    main_raii.cpp)

target_link_libraries(
    HelloWorldRAII
    PRIVATE
    fmt::fmt
    nonstd::expected-lite
    gstreamer::hpp
    GStreamer::Video
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(HelloWorldRAII FALSE "" "" "" "")

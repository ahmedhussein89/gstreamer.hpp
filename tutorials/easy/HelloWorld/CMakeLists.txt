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

add_executable(HelloWorldElements)

target_sources(
    HelloWorldElements
    PRIVATE
    elements.cpp)

target_link_libraries(
    HelloWorldElements
    PRIVATE
    fmt::fmt
    nonstd::expected-lite
    gstreamer::hpp
    GStreamer::Video
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(HelloWorldElements FALSE "" "" "" "")
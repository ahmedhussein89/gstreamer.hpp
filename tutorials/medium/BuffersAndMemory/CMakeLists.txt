# ${CMAKE_SOURCE_DIR}/tutorials/medium/BuffersAndMemory/CMakeLists.txt
add_executable(BuffersAndMemory)
target_sources(BuffersAndMemory PRIVATE main.cpp)
target_link_libraries(BuffersAndMemory PRIVATE fmt::fmt GStreamer::GStreamer ${SELECTED_SANITIZER})
myproject_set_project_warnings(BuffersAndMemory FALSE "" "" "" "")

add_executable(BuffersAndMemoryView)
target_sources(BuffersAndMemoryView PRIVATE main_view.cpp)
target_link_libraries(BuffersAndMemoryView PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
myproject_set_project_warnings(BuffersAndMemoryView FALSE "" "" "" "")

add_executable(BuffersAndMemoryRAII)
target_sources(BuffersAndMemoryRAII PRIVATE main_raii.cpp)
target_link_libraries(BuffersAndMemoryRAII PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
myproject_set_project_warnings(BuffersAndMemoryRAII FALSE "" "" "" "")

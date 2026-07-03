# ${CMAKE_SOURCE_DIR}/tutorials/medium/VideoRecorder/CMakeLists.txt
add_executable(VideoRecorder)
target_sources(VideoRecorder PRIVATE main.cpp)
target_link_libraries(VideoRecorder PRIVATE fmt::fmt GStreamer::GStreamer ${SELECTED_SANITIZER})
myproject_set_project_warnings(VideoRecorder FALSE "" "" "" "")

add_executable(VideoRecorderView)
target_sources(VideoRecorderView PRIVATE main_view.cpp)
target_link_libraries(VideoRecorderView PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
myproject_set_project_warnings(VideoRecorderView FALSE "" "" "" "")

add_executable(VideoRecorderRAII)
target_sources(VideoRecorderRAII PRIVATE main_raii.cpp)
target_link_libraries(VideoRecorderRAII PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
myproject_set_project_warnings(VideoRecorderRAII FALSE "" "" "" "")

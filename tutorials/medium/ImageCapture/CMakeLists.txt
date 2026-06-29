# ${CMAKE_SOURCE_DIR}/tutorials/medium/ImageCapture/CMakeLists.txt
add_executable(ImageCapture)
target_sources(ImageCapture PRIVATE main.cpp)
target_link_libraries(ImageCapture PRIVATE fmt::fmt GStreamer::GStreamer ${SELECTED_SANITIZER})
myproject_set_project_warnings(ImageCapture FALSE "" "" "" "")

add_executable(ImageCaptureRAII)
target_sources(ImageCaptureRAII PRIVATE main_raii.cpp)
target_link_libraries(ImageCaptureRAII PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
myproject_set_project_warnings(ImageCaptureRAII FALSE "" "" "" "")

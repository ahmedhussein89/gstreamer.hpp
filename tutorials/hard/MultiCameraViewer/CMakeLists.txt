# ${CMAKE_SOURCE_DIR}/tutorials/hard/MultiCameraViewer/CMakeLists.txt
add_executable(MultiCameraViewer)
target_sources(MultiCameraViewer PRIVATE main.cpp)
target_link_libraries(MultiCameraViewer PRIVATE fmt::fmt GStreamer::GStreamer ${SELECTED_SANITIZER})
target_link_libraries(MultiCameraViewer PRIVATE gstreamer::warnings)

add_executable(MultiCameraViewerView)
target_sources(MultiCameraViewerView PRIVATE main_view.cpp)
target_link_libraries(MultiCameraViewerView PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
target_link_libraries(MultiCameraViewerView PRIVATE gstreamer::warnings)

add_executable(MultiCameraViewerRAII)
target_sources(MultiCameraViewerRAII PRIVATE main_raii.cpp)
target_link_libraries(MultiCameraViewerRAII PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
target_link_libraries(MultiCameraViewerRAII PRIVATE gstreamer::warnings)

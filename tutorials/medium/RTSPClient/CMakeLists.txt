# ${CMAKE_SOURCE_DIR}/tutorials/medium/RTSPClient/CMakeLists.txt
add_executable(RTSPClient)
target_sources(RTSPClient PRIVATE main.cpp)
target_link_libraries(RTSPClient PRIVATE fmt::fmt GStreamer::GStreamer ${SELECTED_SANITIZER})
myproject_set_project_warnings(RTSPClient FALSE "" "" "" "")

add_executable(RTSPClientView)
target_sources(RTSPClientView PRIVATE main_view.cpp)
target_link_libraries(RTSPClientView PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
myproject_set_project_warnings(RTSPClientView FALSE "" "" "" "")

add_executable(RTSPClientRAII)
target_sources(RTSPClientRAII PRIVATE main_raii.cpp)
target_link_libraries(RTSPClientRAII PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
myproject_set_project_warnings(RTSPClientRAII FALSE "" "" "" "")

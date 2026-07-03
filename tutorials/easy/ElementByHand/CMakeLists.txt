# ${CMAKE_SOURCE_DIR}/tutorials/easy/ElementByHand/CMakeLists.txt
add_executable(ElementByHand)
target_sources(ElementByHand PRIVATE main.cpp)
target_link_libraries(ElementByHand PRIVATE fmt::fmt GStreamer::GStreamer ${SELECTED_SANITIZER})
myproject_set_project_warnings(ElementByHand FALSE "" "" "" "")

add_executable(ElementByHandView)
target_sources(ElementByHandView PRIVATE main_view.cpp)
target_link_libraries(ElementByHandView PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
myproject_set_project_warnings(ElementByHandView FALSE "" "" "" "")

add_executable(ElementByHandRAII)
target_sources(ElementByHandRAII PRIVATE main_raii.cpp)
target_link_libraries(ElementByHandRAII PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
myproject_set_project_warnings(ElementByHandRAII FALSE "" "" "" "")

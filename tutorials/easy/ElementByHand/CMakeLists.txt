# ${CMAKE_SOURCE_DIR}/tutorials/easy/ElementByHand/CMakeLists.txt
add_executable(ElementByHand)
target_sources(ElementByHand PRIVATE main.cpp)
target_link_libraries(ElementByHand PRIVATE fmt::fmt GStreamer::GStreamer ${SELECTED_SANITIZER})
target_link_libraries(ElementByHand PRIVATE deepstream::warnings)

add_executable(ElementByHandView)
target_sources(ElementByHandView PRIVATE main_view.cpp)
target_link_libraries(ElementByHandView PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
target_link_libraries(ElementByHandView PRIVATE deepstream::warnings)

add_executable(ElementByHandRAII)
target_sources(ElementByHandRAII PRIVATE main_raii.cpp)
target_link_libraries(ElementByHandRAII PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
target_link_libraries(ElementByHandRAII PRIVATE deepstream::warnings)

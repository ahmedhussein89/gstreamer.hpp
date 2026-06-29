# ${CMAKE_SOURCE_DIR}/tutorials/medium/CPUVideoProcessing/CMakeLists.txt
add_executable(CPUVideoProcessing)
target_sources(CPUVideoProcessing PRIVATE main.cpp)
target_link_libraries(CPUVideoProcessing PRIVATE fmt::fmt GStreamer::GStreamer ${SELECTED_SANITIZER})
myproject_set_project_warnings(CPUVideoProcessing FALSE "" "" "" "")

add_executable(CPUVideoProcessingRAII)
target_sources(CPUVideoProcessingRAII PRIVATE main_raii.cpp)
target_link_libraries(CPUVideoProcessingRAII PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
myproject_set_project_warnings(CPUVideoProcessingRAII FALSE "" "" "" "")

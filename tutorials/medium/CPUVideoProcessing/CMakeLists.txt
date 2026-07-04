# ${CMAKE_SOURCE_DIR}/tutorials/medium/CPUVideoProcessing/CMakeLists.txt
add_executable(CPUVideoProcessing)
target_sources(CPUVideoProcessing PRIVATE main.cpp)
target_link_libraries(CPUVideoProcessing PRIVATE fmt::fmt GStreamer::GStreamer ${SELECTED_SANITIZER})
target_link_libraries(CPUVideoProcessing PRIVATE deepstream::warnings)

add_executable(CPUVideoProcessingView)
target_sources(CPUVideoProcessingView PRIVATE main_view.cpp)
target_link_libraries(CPUVideoProcessingView PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
target_link_libraries(CPUVideoProcessingView PRIVATE deepstream::warnings)

add_executable(CPUVideoProcessingRAII)
target_sources(CPUVideoProcessingRAII PRIVATE main_raii.cpp)
target_link_libraries(CPUVideoProcessingRAII PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
target_link_libraries(CPUVideoProcessingRAII PRIVATE deepstream::warnings)

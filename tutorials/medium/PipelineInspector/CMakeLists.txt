# ${CMAKE_SOURCE_DIR}/tutorials/medium/PipelineInspector/CMakeLists.txt
add_executable(PipelineInspector)
target_sources(PipelineInspector PRIVATE main.cpp)
target_link_libraries(PipelineInspector PRIVATE fmt::fmt GStreamer::GStreamer ${SELECTED_SANITIZER})
target_link_libraries(PipelineInspector PRIVATE deepstream::warnings)

add_executable(PipelineInspectorView)
target_sources(PipelineInspectorView PRIVATE main_view.cpp)
target_link_libraries(PipelineInspectorView PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
target_link_libraries(PipelineInspectorView PRIVATE deepstream::warnings)

add_executable(PipelineInspectorRAII)
target_sources(PipelineInspectorRAII PRIVATE main_raii.cpp)
target_link_libraries(PipelineInspectorRAII PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
target_link_libraries(PipelineInspectorRAII PRIVATE deepstream::warnings)

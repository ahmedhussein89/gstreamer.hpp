# ${CMAKE_SOURCE_DIR}/tutorials/medium/DynamicPipeline/CMakeLists.txt
add_executable(DynamicPipeline)
target_sources(DynamicPipeline PRIVATE main.cpp)
target_link_libraries(DynamicPipeline PRIVATE fmt::fmt GStreamer::GStreamer ${SELECTED_SANITIZER})
myproject_set_project_warnings(DynamicPipeline FALSE "" "" "" "")

add_executable(DynamicPipelineView)
target_sources(DynamicPipelineView PRIVATE main_view.cpp)
target_link_libraries(DynamicPipelineView PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
myproject_set_project_warnings(DynamicPipelineView FALSE "" "" "" "")

add_executable(DynamicPipelineRAII)
target_sources(DynamicPipelineRAII PRIVATE main_raii.cpp)
target_link_libraries(DynamicPipelineRAII PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
myproject_set_project_warnings(DynamicPipelineRAII FALSE "" "" "" "")

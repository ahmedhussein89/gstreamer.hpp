# ${CMAKE_SOURCE_DIR}/tutorials/medium/TagsAndMetadata/CMakeLists.txt
add_executable(TagsAndMetadata)
target_sources(TagsAndMetadata PRIVATE main.cpp)
target_link_libraries(TagsAndMetadata PRIVATE fmt::fmt GStreamer::GStreamer ${SELECTED_SANITIZER})
target_link_libraries(TagsAndMetadata PRIVATE deepstream::warnings)

add_executable(TagsAndMetadataView)
target_sources(TagsAndMetadataView PRIVATE main_view.cpp)
target_link_libraries(TagsAndMetadataView PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
target_link_libraries(TagsAndMetadataView PRIVATE deepstream::warnings)

add_executable(TagsAndMetadataRAII)
target_sources(TagsAndMetadataRAII PRIVATE main_raii.cpp)
target_link_libraries(TagsAndMetadataRAII PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
target_link_libraries(TagsAndMetadataRAII PRIVATE deepstream::warnings)

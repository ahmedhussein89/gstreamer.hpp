# ${CMAKE_SOURCE_DIR}/tutorials/easy/CapsAndFilters/CMakeLists.txt
add_executable(CapsAndFilters)
target_sources(CapsAndFilters PRIVATE main.cpp)
target_link_libraries(CapsAndFilters PRIVATE fmt::fmt GStreamer::GStreamer ${SELECTED_SANITIZER})
target_link_libraries(CapsAndFilters PRIVATE deepstream::warnings)

add_executable(CapsAndFiltersView)
target_sources(CapsAndFiltersView PRIVATE main_view.cpp)
target_link_libraries(CapsAndFiltersView PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
target_link_libraries(CapsAndFiltersView PRIVATE deepstream::warnings)

add_executable(CapsAndFiltersRAII)
target_sources(CapsAndFiltersRAII PRIVATE main_raii.cpp)
target_link_libraries(CapsAndFiltersRAII PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
target_link_libraries(CapsAndFiltersRAII PRIVATE deepstream::warnings)

# ${CMAKE_SOURCE_DIR}/tutorials/medium/EventsAndQueries/CMakeLists.txt
add_executable(EventsAndQueries)
target_sources(EventsAndQueries PRIVATE main.cpp)
target_link_libraries(EventsAndQueries PRIVATE fmt::fmt GStreamer::GStreamer ${SELECTED_SANITIZER})
target_link_libraries(EventsAndQueries PRIVATE deepstream::warnings)

add_executable(EventsAndQueriesView)
target_sources(EventsAndQueriesView PRIVATE main_view.cpp)
target_link_libraries(EventsAndQueriesView PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
target_link_libraries(EventsAndQueriesView PRIVATE deepstream::warnings)

add_executable(EventsAndQueriesRAII)
target_sources(EventsAndQueriesRAII PRIVATE main_raii.cpp)
target_link_libraries(EventsAndQueriesRAII PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
target_link_libraries(EventsAndQueriesRAII PRIVATE deepstream::warnings)

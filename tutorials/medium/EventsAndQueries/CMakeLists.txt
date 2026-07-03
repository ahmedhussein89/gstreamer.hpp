# ${CMAKE_SOURCE_DIR}/tutorials/medium/EventsAndQueries/CMakeLists.txt
add_executable(EventsAndQueries)
target_sources(EventsAndQueries PRIVATE main.cpp)
target_link_libraries(EventsAndQueries PRIVATE fmt::fmt GStreamer::GStreamer ${SELECTED_SANITIZER})
myproject_set_project_warnings(EventsAndQueries FALSE "" "" "" "")

add_executable(EventsAndQueriesView)
target_sources(EventsAndQueriesView PRIVATE main_view.cpp)
target_link_libraries(EventsAndQueriesView PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
myproject_set_project_warnings(EventsAndQueriesView FALSE "" "" "" "")

add_executable(EventsAndQueriesRAII)
target_sources(EventsAndQueriesRAII PRIVATE main_raii.cpp)
target_link_libraries(EventsAndQueriesRAII PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
myproject_set_project_warnings(EventsAndQueriesRAII FALSE "" "" "" "")

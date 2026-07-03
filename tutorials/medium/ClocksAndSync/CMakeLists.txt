# ${CMAKE_SOURCE_DIR}/tutorials/medium/ClocksAndSync/CMakeLists.txt
add_executable(ClocksAndSync)
target_sources(ClocksAndSync PRIVATE main.cpp)
target_link_libraries(ClocksAndSync PRIVATE fmt::fmt GStreamer::GStreamer ${SELECTED_SANITIZER})
myproject_set_project_warnings(ClocksAndSync FALSE "" "" "" "")

add_executable(ClocksAndSyncView)
target_sources(ClocksAndSyncView PRIVATE main_view.cpp)
target_link_libraries(ClocksAndSyncView PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
myproject_set_project_warnings(ClocksAndSyncView FALSE "" "" "" "")

add_executable(ClocksAndSyncRAII)
target_sources(ClocksAndSyncRAII PRIVATE main_raii.cpp)
target_link_libraries(ClocksAndSyncRAII PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
myproject_set_project_warnings(ClocksAndSyncRAII FALSE "" "" "" "")

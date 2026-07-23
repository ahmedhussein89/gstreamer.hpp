# ${CMAKE_SOURCE_DIR}/tutorials/hard/NetClockSync/CMakeLists.txt
if(NOT TARGET GStreamer::Net)
  message(STATUS "Skipping NetClockSync tutorial: GStreamer net library not found")
  return()
endif()

add_executable(NetClockSync)
target_sources(NetClockSync PRIVATE main.cpp)
target_link_libraries(NetClockSync PRIVATE fmt::fmt GStreamer::GStreamer GStreamer::Net ${SELECTED_SANITIZER})
target_link_libraries(NetClockSync PRIVATE gstreamer::warnings)

add_executable(NetClockSyncView)
target_sources(NetClockSyncView PRIVATE main_view.cpp)
target_link_libraries(NetClockSyncView PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video GStreamer::Net ${SELECTED_SANITIZER})
target_link_libraries(NetClockSyncView PRIVATE gstreamer::warnings)

add_executable(NetClockSyncRAII)
target_sources(NetClockSyncRAII PRIVATE main_raii.cpp)
target_link_libraries(NetClockSyncRAII PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video GStreamer::Net ${SELECTED_SANITIZER})
target_link_libraries(NetClockSyncRAII PRIVATE gstreamer::warnings)

# ${CMAKE_SOURCE_DIR}/tutorials/hard/RTSPServer/CMakeLists.txt
if(NOT TARGET GStreamer::RtspServer)
  message(STATUS "Skipping RTSPServer tutorial: gst-rtsp-server not found")
  return()
endif()

add_executable(RTSPServer)
target_sources(RTSPServer PRIVATE main.cpp)
target_link_libraries(RTSPServer PRIVATE fmt::fmt GStreamer::GStreamer GStreamer::RtspServer ${SELECTED_SANITIZER})
target_link_libraries(RTSPServer PRIVATE gstreamer::warnings)

add_executable(RTSPServerView)
target_sources(RTSPServerView PRIVATE main_view.cpp)
target_link_libraries(RTSPServerView PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::RtspServer ${SELECTED_SANITIZER})
target_link_libraries(RTSPServerView PRIVATE gstreamer::warnings)

add_executable(RTSPServerRAII)
target_sources(RTSPServerRAII PRIVATE main_raii.cpp)
target_link_libraries(RTSPServerRAII PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::RtspServer ${SELECTED_SANITIZER})
target_link_libraries(RTSPServerRAII PRIVATE gstreamer::warnings)

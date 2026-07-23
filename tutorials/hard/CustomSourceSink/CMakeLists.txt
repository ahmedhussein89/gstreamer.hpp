# ${CMAKE_SOURCE_DIR}/tutorials/hard/CustomSourceSink/CMakeLists.txt
if(NOT TARGET GStreamer::Base)
  message(STATUS "Skipping CustomSourceSink tutorial: GStreamer base library not found")
  return()
endif()

add_executable(CustomSourceSink)
target_sources(CustomSourceSink PRIVATE main.cpp mysrc.c mysink.c)
target_link_libraries(CustomSourceSink PRIVATE fmt::fmt GStreamer::GStreamer GStreamer::Base ${SELECTED_SANITIZER})
target_link_libraries(CustomSourceSink PRIVATE gstreamer::warnings)

add_executable(CustomSourceSinkView)
target_sources(CustomSourceSinkView PRIVATE main_view.cpp mysrc.c mysink.c)
target_link_libraries(CustomSourceSinkView PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Base ${SELECTED_SANITIZER})
target_link_libraries(CustomSourceSinkView PRIVATE gstreamer::warnings)

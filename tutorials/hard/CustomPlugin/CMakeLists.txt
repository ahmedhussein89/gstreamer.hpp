# ${CMAKE_SOURCE_DIR}/tutorials/hard/CustomPlugin/CMakeLists.txt
if(NOT TARGET GStreamer::Base)
  message(STATUS "Skipping CustomPlugin tutorial: GStreamer base library not found")
  return()
endif()

add_executable(CustomPlugin)
target_sources(CustomPlugin PRIVATE main.cpp myedgedetector.c)
target_link_libraries(CustomPlugin PRIVATE fmt::fmt GStreamer::GStreamer GStreamer::Base gstreamer::warnings ${SELECTED_SANITIZER})

add_executable(CustomPluginView)
target_sources(CustomPluginView PRIVATE main_view.cpp myedgedetector.c)
target_link_libraries(CustomPluginView PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Base gstreamer::warnings ${SELECTED_SANITIZER})

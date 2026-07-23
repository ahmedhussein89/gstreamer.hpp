# ${CMAKE_SOURCE_DIR}/tutorials/hard/TestingElements/CMakeLists.txt
if(NOT TARGET GStreamer::Check OR NOT TARGET GStreamer::Base)
  message(STATUS "Skipping TestingElements tutorial: gstcheck or gstbase not found")
  return()
endif()

add_executable(TestingElements)
target_sources(TestingElements PRIVATE main.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../CustomPlugin/myedgedetector.c)
target_include_directories(TestingElements PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../CustomPlugin)
target_link_libraries(TestingElements PRIVATE fmt::fmt GStreamer::GStreamer GStreamer::Base GStreamer::Check ${SELECTED_SANITIZER})
target_link_libraries(TestingElements PRIVATE gstreamer::warnings)

add_executable(TestingElementsView)
target_sources(TestingElementsView PRIVATE main_view.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../CustomPlugin/myedgedetector.c)
target_include_directories(TestingElementsView PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../CustomPlugin)
target_link_libraries(TestingElementsView PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Base GStreamer::Check ${SELECTED_SANITIZER})
target_link_libraries(TestingElementsView PRIVATE gstreamer::warnings)

add_test(NAME TestingElements COMMAND TestingElements)
add_test(NAME TestingElementsView COMMAND TestingElementsView)

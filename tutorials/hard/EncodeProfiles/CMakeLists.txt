if(NOT TARGET GStreamer::Pbutils)
  message(STATUS "Skipping EncodeProfiles tutorial: GStreamer pbutils not found")
  return()
endif()

add_executable(EncodeProfiles)
target_sources(EncodeProfiles PRIVATE main.cpp)
target_link_libraries(EncodeProfiles PRIVATE fmt::fmt GStreamer::GStreamer GStreamer::Pbutils ${SELECTED_SANITIZER})
target_link_libraries(EncodeProfiles PRIVATE gstreamer::warnings)

add_executable(EncodeProfilesView)
target_sources(EncodeProfilesView PRIVATE main_view.cpp)
target_link_libraries(EncodeProfilesView PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video GStreamer::Pbutils ${SELECTED_SANITIZER})
target_link_libraries(EncodeProfilesView PRIVATE gstreamer::warnings)

add_executable(EncodeProfilesRAII)
target_sources(EncodeProfilesRAII PRIVATE main_raii.cpp)
target_link_libraries(EncodeProfilesRAII PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video GStreamer::Pbutils ${SELECTED_SANITIZER})
target_link_libraries(EncodeProfilesRAII PRIVATE gstreamer::warnings)

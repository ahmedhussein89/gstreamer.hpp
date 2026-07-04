# ${CMAKE_SOURCE_DIR}/tutorials/easy/StatesAndSeeking/CMakeLists.txt
add_executable(StatesAndSeeking)
target_sources(StatesAndSeeking PRIVATE main.cpp)
target_link_libraries(StatesAndSeeking PRIVATE fmt::fmt GStreamer::GStreamer ${SELECTED_SANITIZER})
target_link_libraries(StatesAndSeeking PRIVATE deepstream::warnings)

add_executable(StatesAndSeekingView)
target_sources(StatesAndSeekingView PRIVATE main_view.cpp)
target_link_libraries(StatesAndSeekingView PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
target_link_libraries(StatesAndSeekingView PRIVATE deepstream::warnings)

add_executable(StatesAndSeekingRAII)
target_sources(StatesAndSeekingRAII PRIVATE main_raii.cpp)
target_link_libraries(StatesAndSeekingRAII PRIVATE fmt::fmt nonstd::expected-lite gstreamer::hpp GStreamer::Video ${SELECTED_SANITIZER})
target_link_libraries(StatesAndSeekingRAII PRIVATE deepstream::warnings)

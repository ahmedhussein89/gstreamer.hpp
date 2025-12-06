# ${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt
find_package(GTest REQUIRED)

add_executable(
    testGstreamer
    testGstreamer.cpp)

target_link_libraries(
    testGstreamer
    PRIVATE
    GTest::GTest
    GTest::Main
    gstreamer::hpp
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(testGstreamer TRUE "" "" "" "")

gtest_discover_tests(testGstreamer)

# ============================================================================
# Coverage Targets
# ============================================================================
if(ENABLE_COVERAGE)
  add_coverage_reports(
    TARGET_NAME coverage
    EXECUTABLE ${CMAKE_BINARY_DIR}/tests/testGstreamer
    EXCLUDE "/usr/*" "build/*" "cmake/*" "_deps/*"
  )

  add_coverage_summary(
    TARGET_NAME coverage-summary
    EXCLUDE "/usr/*" "build/*" "cmake/*" "_deps/*"
  )
endif()

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

add_executable(
    testPipeline
    testPipeline.cpp)

target_link_libraries(
    testPipeline
    PRIVATE
    GTest::GTest
    GTest::Main
    pipeline::hpp
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(testPipeline TRUE "" "" "" "")

gtest_discover_tests(testPipeline)

add_executable(
    testBuilder
    testBuilder.cpp)

target_link_libraries(
    testBuilder
    PRIVATE
    GTest::GTest
    GTest::Main
    ds::elements
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(testBuilder TRUE "" "" "" "")

gtest_discover_tests(testBuilder)

add_executable(
    testElements
    testElements.cpp)

target_link_libraries(
    testElements
    PRIVATE
    GTest::GTest
    GTest::Main
    ds::elements
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(testElements TRUE "" "" "" "")

gtest_discover_tests(testElements)

add_executable(
    testDebug
    testDebug.cpp)

target_link_libraries(
    testDebug
    PRIVATE
    GTest::GTest
    GTest::Main
    ds::elements
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(testDebug TRUE "" "" "" "")

gtest_discover_tests(testDebug)

if(DeepStream_FOUND)
  add_executable(
      testMetadata
      testMetadata.cpp)

  target_link_libraries(
      testMetadata
      PRIVATE
      GTest::GTest
      GTest::Main
      ds::metadata
      ${SELECTED_SANITIZER})

  myproject_set_project_warnings(testMetadata TRUE "" "" "" "")

  gtest_discover_tests(testMetadata)
endif()

add_executable(
    testConcepts
    testConcepts.cpp)

target_link_libraries(
    testConcepts
    PRIVATE
    GTest::GTest
    GTest::Main
    pipeline::hpp
    ds::elements
    ${SELECTED_SANITIZER})

myproject_set_project_warnings(testConcepts TRUE "" "" "" "")

gtest_discover_tests(testConcepts)

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

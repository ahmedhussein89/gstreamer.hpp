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

target_link_libraries(testGstreamer PRIVATE deepstream::warnings_strict)

gtest_discover_tests(testGstreamer)

add_executable(
    testPipeline
    testPipeline.cpp)

target_link_libraries(
    testPipeline
    PRIVATE
    GTest::GTest
    GTest::Main
    gstreamer::hpp
    ${SELECTED_SANITIZER})

target_link_libraries(testPipeline PRIVATE deepstream::warnings_strict)

gtest_discover_tests(testPipeline)

add_executable(
    testBuilder
    testBuilder.cpp)

target_link_libraries(
    testBuilder
    PRIVATE
    GTest::GTest
    GTest::Main
    ds::raii
    ${SELECTED_SANITIZER})

target_link_libraries(testBuilder PRIVATE deepstream::warnings_strict)

gtest_discover_tests(testBuilder)

add_executable(
    testElements
    testElements.cpp)

target_link_libraries(
    testElements
    PRIVATE
    GTest::GTest
    GTest::Main
    ds::raii
    ${SELECTED_SANITIZER})

target_link_libraries(testElements PRIVATE deepstream::warnings_strict)

gtest_discover_tests(testElements)

add_executable(
    testDebug
    testDebug.cpp)

target_link_libraries(
    testDebug
    PRIVATE
    GTest::GTest
    GTest::Main
    ds::raii
    ${SELECTED_SANITIZER})

target_link_libraries(testDebug PRIVATE deepstream::warnings_strict)

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
      ds::hpp
      ${SELECTED_SANITIZER})

  target_link_libraries(testMetadata PRIVATE deepstream::warnings_strict)

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
    gstreamer::hpp
    ds::raii
    ${SELECTED_SANITIZER})

target_link_libraries(testConcepts PRIVATE deepstream::warnings_strict)

gtest_discover_tests(testConcepts)

add_executable(
    testCore
    testCore.cpp)

target_link_libraries(
    testCore
    PRIVATE
    GTest::GTest
    GTest::Main
    gstreamer::hpp
    ${SELECTED_SANITIZER})

target_link_libraries(testCore PRIVATE deepstream::warnings_strict)

gtest_discover_tests(testCore)

add_executable(
    testGstreamerRaii
    testGstreamerRaii.cpp)

target_link_libraries(
    testGstreamerRaii
    PRIVATE
    GTest::GTest
    GTest::Main
    gstreamer::hpp
    ${SELECTED_SANITIZER})

target_link_libraries(testGstreamerRaii PRIVATE deepstream::warnings_strict)

gtest_discover_tests(testGstreamerRaii)

# ============================================================================
# Coverage Targets
# ============================================================================
if(ENABLE_COVERAGE)
  set(_cov_excludes
      "--exclude=.*/_deps/.*"
      "--exclude=/usr/.*"
      "--exclude=.*build.*/_deps/.*"
      "--exclude=.*/tests/.*")

  set(_cov_out "${CMAKE_BINARY_DIR}/coverage-reports")

  add_custom_target(coverage
    COMMAND ${CMAKE_COMMAND} -E make_directory ${_cov_out}/text ${_cov_out}/html ${_cov_out}/xml
    COMMAND ${CMAKE_BINARY_DIR}/tests/testGstreamer
    COMMAND ${CMAKE_BINARY_DIR}/tests/testGstreamerRaii
    COMMAND ${CMAKE_BINARY_DIR}/tests/testCore
    COMMAND ${CMAKE_BINARY_DIR}/tests/testPipeline
    COMMAND ${CMAKE_BINARY_DIR}/tests/testBuilder
    COMMAND ${CMAKE_BINARY_DIR}/tests/testElements
    COMMAND ${CMAKE_BINARY_DIR}/tests/testDebug
    COMMAND ${CMAKE_BINARY_DIR}/tests/testConcepts
    COMMAND ${GCOVR_EXECUTABLE}
      --root=${CMAKE_SOURCE_DIR}
      --object-directory=${CMAKE_BINARY_DIR}
      --output=${_cov_out}/text/coverage.txt
      --print-summary
      --sort-percentage
      ${_cov_excludes}
    COMMAND ${GCOVR_EXECUTABLE}
      --root=${CMAKE_SOURCE_DIR}
      --object-directory=${CMAKE_BINARY_DIR}
      --html-details=${_cov_out}/html/index.html
      --sort-percentage
      ${_cov_excludes}
    COMMAND ${GCOVR_EXECUTABLE}
      --root=${CMAKE_SOURCE_DIR}
      --object-directory=${CMAKE_BINARY_DIR}
      --xml=${_cov_out}/xml/coverage.xml
      --sort-percentage
      ${_cov_excludes}
    COMMENT "Running all tests and generating coverage reports"
    VERBATIM)

  add_custom_target(coverage-summary
    COMMAND ${GCOVR_EXECUTABLE}
      --root=${CMAKE_SOURCE_DIR}
      --object-directory=${CMAKE_BINARY_DIR}
      --print-summary
      --sort-percentage
      ${_cov_excludes}
    COMMENT "Displaying coverage summary"
    VERBATIM)
endif()

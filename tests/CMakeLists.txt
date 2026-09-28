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

target_link_libraries(testGstreamer PRIVATE gstreamer::warnings_strict)

gtest_discover_tests(testGstreamer DISCOVERY_MODE PRE_TEST DISCOVERY_TIMEOUT 60)

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

target_link_libraries(testPipeline PRIVATE gstreamer::warnings_strict)

gtest_discover_tests(testPipeline DISCOVERY_MODE PRE_TEST DISCOVERY_TIMEOUT 60)

add_executable(
    testConcepts
    testConcepts.cpp)

target_link_libraries(
    testConcepts
    PRIVATE
    GTest::GTest
    GTest::Main
    gstreamer::hpp
    ${SELECTED_SANITIZER})

target_link_libraries(testConcepts PRIVATE gstreamer::warnings_strict)

gtest_discover_tests(testConcepts DISCOVERY_MODE PRE_TEST DISCOVERY_TIMEOUT 60)

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

target_link_libraries(testCore PRIVATE gstreamer::warnings_strict)

gtest_discover_tests(testCore DISCOVERY_MODE PRE_TEST DISCOVERY_TIMEOUT 60)

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

target_link_libraries(testGstreamerRaii PRIVATE gstreamer::warnings_strict)

gtest_discover_tests(testGstreamerRaii DISCOVERY_MODE PRE_TEST DISCOVERY_TIMEOUT 60)

if(ENABLE_COVERAGE)
  add_coverage_reports(
    TARGET_NAME coverage
    OUTPUT_DIR ${PROJECT_BINARY_DIR}/coverage-reports
    DEPENDS testGstreamer testGstreamerRaii testCore testPipeline testConcepts
    FILTER ${PROJECT_SOURCE_DIR}/include/)

  add_coverage_summary(
    TARGET_NAME coverage-summary
    FILTER ${PROJECT_SOURCE_DIR}/include/)
endif()

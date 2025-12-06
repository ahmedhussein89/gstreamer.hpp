# Coverage configuration for clang and gcc compilers using gcovr
# This module provides targets and functions for code coverage analysis
# Note: Coverage is applied only to project source, excluding all FetchContent dependencies

include_guard(GLOBAL)

# Check if coverage is enabled
option(ENABLE_COVERAGE "Enable code coverage" OFF)

if(NOT ENABLE_COVERAGE)
  return()
endif()

# Verify gcovr is available
find_program(GCOVR_EXECUTABLE gcovr)
if(NOT GCOVR_EXECUTABLE)
  message(WARNING "Coverage enabled but gcovr not found. Install with: pip install gcovr")
  message(WARNING "Coverage features will not be available")
  return()
endif()

message(STATUS "Found gcovr: ${GCOVR_EXECUTABLE}")

# Detect compiler
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(COMPILER_IS_GCC TRUE)
  message(STATUS "Coverage: Configuring for GCC compiler")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(COMPILER_IS_CLANG TRUE)
  message(STATUS "Coverage: Configuring for Clang compiler")
else()
  message(WARNING "Coverage: Unsupported compiler ${CMAKE_CXX_COMPILER_ID}")
  return()
endif()

# ============================================================================
# Compiler Flags Configuration
# ============================================================================

# Common coverage flags for both GCC and Clang
set(COVERAGE_COMPILE_FLAGS "-fprofile-arcs -ftest-coverage")
set(COVERAGE_LINK_FLAGS "-lgcov")

if(COMPILER_IS_CLANG)
  # Clang-specific coverage options
  set(COVERAGE_COMPILE_FLAGS "${COVERAGE_COMPILE_FLAGS} --coverage")
  set(COVERAGE_LINK_FLAGS "--coverage")
  message(STATUS "Coverage: Using Clang coverage flags")
elseif(COMPILER_IS_GCC)
  # GCC-specific coverage options
  set(COVERAGE_COMPILE_FLAGS "${COVERAGE_COMPILE_FLAGS} --coverage")
  set(COVERAGE_LINK_FLAGS "--coverage")
  message(STATUS "Coverage: Using GCC coverage flags")
endif()

# Apply flags to CMAKE variables
string(APPEND CMAKE_CXX_FLAGS " ${COVERAGE_COMPILE_FLAGS}")
string(APPEND CMAKE_C_FLAGS " ${COVERAGE_COMPILE_FLAGS}")
string(APPEND CMAKE_EXE_LINKER_FLAGS " ${COVERAGE_LINK_FLAGS}")
string(APPEND CMAKE_SHARED_LINKER_FLAGS " ${COVERAGE_LINK_FLAGS}")

message(STATUS "Coverage: CXX Flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "Coverage: EXE Linker Flags: ${CMAKE_EXE_LINKER_FLAGS}")

# ============================================================================
# Coverage Report Generation Functions
# ============================================================================

# Function to add coverage target for a specific test executable
# Usage: add_coverage_target(
#   TARGET_NAME <name>
#   EXECUTABLE <path_to_test_executable>
#   [EXCLUDE <exclude_patterns>...]
#   [OUTPUT_DIR <directory>]
# )
function(add_coverage_target)
  cmake_parse_arguments(
    COVERAGE
    ""
    "TARGET_NAME;EXECUTABLE;OUTPUT_DIR"
    "EXCLUDE"
    ${ARGN}
  )

  if(NOT COVERAGE_TARGET_NAME)
    message(FATAL_ERROR "add_coverage_target: TARGET_NAME is required")
  endif()

  if(NOT COVERAGE_EXECUTABLE)
    message(FATAL_ERROR "add_coverage_target: EXECUTABLE is required")
  endif()

  if(NOT COVERAGE_OUTPUT_DIR)
    set(COVERAGE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/coverage")
  endif()

  # Build exclude patterns list - exclude FetchContent and system paths
  set(EXCLUDE_ARGS "")
  # Always exclude FetchContent dependencies
  list(APPEND EXCLUDE_ARGS "--exclude=.*/_deps/.*")
  list(APPEND EXCLUDE_ARGS "--exclude=/usr/.*")
  list(APPEND EXCLUDE_ARGS "--exclude=.*build.*/_deps/.*")
  foreach(pattern ${COVERAGE_EXCLUDE})
    list(APPEND EXCLUDE_ARGS "--exclude=${pattern}")
  endforeach()

  # Add custom command to generate coverage report
  add_custom_target(${COVERAGE_TARGET_NAME}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_OUTPUT_DIR}
    COMMAND ${GCOVR_EXECUTABLE}
      --root=${CMAKE_SOURCE_DIR}
      --object-directory=${CMAKE_BINARY_DIR}
      --output=${COVERAGE_OUTPUT_DIR}/coverage.txt
      --print-summary
      --sort-percentage
      ${EXCLUDE_ARGS}
    COMMENT "Generating coverage report for ${COVERAGE_TARGET_NAME}"
    VERBATIM
  )
endfunction()

# ============================================================================
# HTML Coverage Report Generation Functions
# ============================================================================

# Function to add HTML coverage report target
# Usage: add_coverage_html_target(
#   TARGET_NAME <name>
#   EXECUTABLE <path_to_test_executable>
#   [EXCLUDE <exclude_patterns>...]
#   [OUTPUT_DIR <directory>]
# )
function(add_coverage_html_target)
  cmake_parse_arguments(
    COVERAGE_HTML
    ""
    "TARGET_NAME;EXECUTABLE;OUTPUT_DIR"
    "EXCLUDE"
    ${ARGN}
  )

  if(NOT COVERAGE_HTML_TARGET_NAME)
    message(FATAL_ERROR "add_coverage_html_target: TARGET_NAME is required")
  endif()

  if(NOT COVERAGE_HTML_EXECUTABLE)
    message(FATAL_ERROR "add_coverage_html_target: EXECUTABLE is required")
  endif()

  if(NOT COVERAGE_HTML_OUTPUT_DIR)
    set(COVERAGE_HTML_OUTPUT_DIR "${CMAKE_BINARY_DIR}/coverage-html")
  endif()

  # Build exclude patterns list - exclude FetchContent and system paths
  set(EXCLUDE_ARGS "")
  # Always exclude FetchContent dependencies
  list(APPEND EXCLUDE_ARGS "--exclude=.*/_deps/.*")
  list(APPEND EXCLUDE_ARGS "--exclude=/usr/.*")
  list(APPEND EXCLUDE_ARGS "--exclude=.*build.*/_deps/.*")
  foreach(pattern ${COVERAGE_HTML_EXCLUDE})
    list(APPEND EXCLUDE_ARGS "--exclude=${pattern}")
  endforeach()

  # Add custom command to generate HTML coverage report
  add_custom_target(${COVERAGE_HTML_TARGET_NAME}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_HTML_OUTPUT_DIR}
    COMMAND ${GCOVR_EXECUTABLE}
      --root=${CMAKE_SOURCE_DIR}
      --object-directory=${CMAKE_BINARY_DIR}
      --html-details=${COVERAGE_HTML_OUTPUT_DIR}/index.html
      --sort-percentage
      ${EXCLUDE_ARGS}
    COMMAND ${CMAKE_COMMAND} -E echo "HTML coverage report generated at: ${COVERAGE_HTML_OUTPUT_DIR}/index.html"
    COMMENT "Generating HTML coverage report for ${COVERAGE_HTML_TARGET_NAME}"
    VERBATIM
  )
endfunction()

# ============================================================================
# XML Coverage Report Generation Functions (for CI/CD integration)
# ============================================================================

# Function to add XML coverage report target
# Usage: add_coverage_xml_target(
#   TARGET_NAME <name>
#   EXECUTABLE <path_to_test_executable>
#   [EXCLUDE <exclude_patterns>...]
#   [OUTPUT_DIR <directory>]
# )
function(add_coverage_xml_target)
  cmake_parse_arguments(
    COVERAGE_XML
    ""
    "TARGET_NAME;EXECUTABLE;OUTPUT_DIR"
    "EXCLUDE"
    ${ARGN}
  )

  if(NOT COVERAGE_XML_TARGET_NAME)
    message(FATAL_ERROR "add_coverage_xml_target: TARGET_NAME is required")
  endif()

  if(NOT COVERAGE_XML_EXECUTABLE)
    message(FATAL_ERROR "add_coverage_xml_target: EXECUTABLE is required")
  endif()

  if(NOT COVERAGE_XML_OUTPUT_DIR)
    set(COVERAGE_XML_OUTPUT_DIR "${CMAKE_BINARY_DIR}/coverage-xml")
  endif()

  # Build exclude patterns list - exclude FetchContent and system paths
  set(EXCLUDE_ARGS "")
  # Always exclude FetchContent dependencies
  list(APPEND EXCLUDE_ARGS "--exclude=.*/_deps/.*")
  list(APPEND EXCLUDE_ARGS "--exclude=/usr/.*")
  list(APPEND EXCLUDE_ARGS "--exclude=.*build.*/_deps/.*")
  foreach(pattern ${COVERAGE_XML_EXCLUDE})
    list(APPEND EXCLUDE_ARGS "--exclude=${pattern}")
  endforeach()

  # Add custom command to generate XML coverage report
  add_custom_target(${COVERAGE_XML_TARGET_NAME}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_XML_OUTPUT_DIR}
    COMMAND ${GCOVR_EXECUTABLE}
      --root=${CMAKE_SOURCE_DIR}
      --object-directory=${CMAKE_BINARY_DIR}
      --xml=${COVERAGE_XML_OUTPUT_DIR}/coverage.xml
      --sort-percentage
      ${EXCLUDE_ARGS}
    COMMAND ${CMAKE_COMMAND} -E echo "XML coverage report generated at: ${COVERAGE_XML_OUTPUT_DIR}/coverage.xml"
    COMMENT "Generating XML coverage report for ${COVERAGE_XML_TARGET_NAME}"
    VERBATIM
  )
endfunction()

# ============================================================================
# Unified Coverage Report Generation (All Formats)
# ============================================================================

# Function to add comprehensive coverage report target
# Generates text, HTML, and XML reports in one target
# Usage: add_coverage_reports(
#   TARGET_NAME <name>
#   EXECUTABLE <path_to_test_executable>
#   [EXCLUDE <exclude_patterns>...]
#   [OUTPUT_DIR <directory>]
# )
function(add_coverage_reports)
  cmake_parse_arguments(
    COV_REPORTS
    ""
    "TARGET_NAME;EXECUTABLE;OUTPUT_DIR"
    "EXCLUDE"
    ${ARGN}
  )

  if(NOT COV_REPORTS_TARGET_NAME)
    message(FATAL_ERROR "add_coverage_reports: TARGET_NAME is required")
  endif()

  if(NOT COV_REPORTS_EXECUTABLE)
    message(FATAL_ERROR "add_coverage_reports: EXECUTABLE is required")
  endif()

  if(NOT COV_REPORTS_OUTPUT_DIR)
    set(COV_REPORTS_OUTPUT_DIR "${CMAKE_BINARY_DIR}/coverage-reports")
  endif()

  # Build exclude patterns list - exclude FetchContent and system paths
  set(EXCLUDE_ARGS "")
  # Always exclude FetchContent dependencies
  list(APPEND EXCLUDE_ARGS "--exclude=.*/_deps/.*")
  list(APPEND EXCLUDE_ARGS "--exclude=/usr/.*")
  list(APPEND EXCLUDE_ARGS "--exclude=.*build.*/_deps/.*")
  foreach(pattern ${COV_REPORTS_EXCLUDE})
    list(APPEND EXCLUDE_ARGS "--exclude=${pattern}")
  endforeach()

  # Create output subdirectories
  set(TEXT_DIR "${COV_REPORTS_OUTPUT_DIR}/text")
  set(HTML_DIR "${COV_REPORTS_OUTPUT_DIR}/html")
  set(XML_DIR "${COV_REPORTS_OUTPUT_DIR}/xml")

  # Add comprehensive custom target
  add_custom_target(${COV_REPORTS_TARGET_NAME}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TEXT_DIR} ${HTML_DIR} ${XML_DIR}
    # Text report
    COMMAND ${GCOVR_EXECUTABLE}
      --root=${CMAKE_SOURCE_DIR}
      --object-directory=${CMAKE_BINARY_DIR}
      --output=${TEXT_DIR}/coverage.txt
      --print-summary
      --sort-percentage
      ${EXCLUDE_ARGS}
    # HTML report
    COMMAND ${GCOVR_EXECUTABLE}
      --root=${CMAKE_SOURCE_DIR}
      --object-directory=${CMAKE_BINARY_DIR}
      --html-details=${HTML_DIR}/index.html
      --sort-percentage
      ${EXCLUDE_ARGS}
    # XML report
    COMMAND ${GCOVR_EXECUTABLE}
      --root=${CMAKE_SOURCE_DIR}
      --object-directory=${CMAKE_BINARY_DIR}
      --xml=${XML_DIR}/coverage.xml
      --sort-percentage
      ${EXCLUDE_ARGS}
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Coverage reports generated:"
    COMMAND ${CMAKE_COMMAND} -E echo "  Text: ${TEXT_DIR}/coverage.txt"
    COMMAND ${CMAKE_COMMAND} -E echo "  HTML: ${HTML_DIR}/index.html"
    COMMAND ${CMAKE_COMMAND} -E echo "  XML:  ${XML_DIR}/coverage.xml"
    COMMENT "Generating comprehensive coverage reports"
    VERBATIM
  )
endfunction()

# ============================================================================
# Coverage Summary (Display only, no file output)
# ============================================================================

# Function to add coverage summary target that displays coverage without saving
# Usage: add_coverage_summary(
#   TARGET_NAME <name>
#   [EXCLUDE <exclude_patterns>...]
# )
function(add_coverage_summary)
  cmake_parse_arguments(
    COV_SUMMARY
    ""
    "TARGET_NAME"
    "EXCLUDE"
    ${ARGN}
  )

  if(NOT COV_SUMMARY_TARGET_NAME)
    message(FATAL_ERROR "add_coverage_summary: TARGET_NAME is required")
  endif()

  # Build exclude patterns list - exclude FetchContent and system paths
  set(EXCLUDE_ARGS "")
  # Always exclude FetchContent dependencies
  list(APPEND EXCLUDE_ARGS "--exclude=.*/_deps/.*")
  list(APPEND EXCLUDE_ARGS "--exclude=/usr/.*")
  list(APPEND EXCLUDE_ARGS "--exclude=.*build.*/_deps/.*")
  foreach(pattern ${COV_SUMMARY_EXCLUDE})
    list(APPEND EXCLUDE_ARGS "--exclude=${pattern}")
  endforeach()

  # Add custom target for coverage summary
  add_custom_target(${COV_SUMMARY_TARGET_NAME}
    COMMAND ${GCOVR_EXECUTABLE}
      --root=${CMAKE_SOURCE_DIR}
      --object-directory=${CMAKE_BINARY_DIR}
      --print-summary
      --sort-percentage
      ${EXCLUDE_ARGS}
    COMMENT "Displaying coverage summary"
    VERBATIM
  )
endfunction()

# ============================================================================
# Configuration Summary
# ============================================================================

message(STATUS "")
message(STATUS "====== Code Coverage Configuration ======")
message(STATUS "Coverage enabled: ON")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "gcovr path: ${GCOVR_EXECUTABLE}")
message(STATUS "")
message(STATUS "To use coverage:")
message(STATUS "  1. Add coverage targets in your CMakeLists.txt:")
message(STATUS "     add_coverage_reports(")
message(STATUS "       TARGET_NAME coverage")
message(STATUS "       EXECUTABLE \${CMAKE_BINARY_DIR}/tests/testGstreamer")
message(STATUS "       EXCLUDE '/usr/*' 'build/*'")
message(STATUS "     )")
message(STATUS "")
message(STATUS "  2. Build with coverage:")
message(STATUS "     cmake -DENABLE_COVERAGE=ON -B build")
message(STATUS "     cmake --build build")
message(STATUS "")
message(STATUS "  3. Run tests and generate reports:")
message(STATUS "     ./build/tests/testGstreamer")
message(STATUS "     cmake --build build -t coverage")
message(STATUS "")
message(STATUS "Available targets (after adding coverage targets):")
message(STATUS "  - coverage (or custom name): Generate all reports")
message(STATUS "  - coverage-text: Text report only")
message(STATUS "  - coverage-html: HTML report only")
message(STATUS "  - coverage-xml: XML report only")
message(STATUS "  - coverage-summary: Console summary only")
message(STATUS "=========================================")
message(STATUS "")

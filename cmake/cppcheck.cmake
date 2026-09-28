# cmake/cppcheck.cmake
find_program(CPPCHECK_EXECUTABLE NAMES cppcheck)

if(NOT CPPCHECK_EXECUTABLE)
  message(FATAL_ERROR "GST_ENABLE_CPPCHECK is ON but cppcheck was not found")
endif()

message(STATUS "cppcheck enabled: ${CPPCHECK_EXECUTABLE}")

set(CMAKE_CXX_CPPCHECK
    "${CPPCHECK_EXECUTABLE}"
    "--enable=warning,performance,portability"
    "--std=c++20"
    "--suppress=missingInclude"
    "--inline-suppr"
    CACHE STRING "cppcheck command" FORCE)

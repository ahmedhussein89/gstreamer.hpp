# cmake/clang_tidy.cmake
find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy-20 clang-tidy)

if(NOT CLANG_TIDY_EXECUTABLE)
  message(FATAL_ERROR "DS_ENABLE_CLANG_TIDY is ON but clang-tidy was not found")
endif()

message(STATUS "clang-tidy enabled: ${CLANG_TIDY_EXECUTABLE}")

set(CMAKE_CXX_CLANG_TIDY
    "${CLANG_TIDY_EXECUTABLE}"
    "--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy"
    CACHE STRING "clang-tidy command" FORCE)

cmake_minimum_required(VERSION 3.15)

project(bzm C)

# Explicitly drop Windows support for now
if(WIN32)
  message(FATAL_ERROR "Windows is not supported for this project at the moment. Please use Linux or macOS.")
endif()

option(BZM_BUILD_TESTS "Build unit tests" ON)
option(BZM_ENABLE_ASAN "Enable AddressSanitizer for tests" OFF)
option(BZM_WERROR "Treat warnings as errors" ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(bzm
    src/bzm_commands.c
)
target_include_directories(bzm PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_definitions(bzm PRIVATE $<$<CONFIG:Debug>:BZM_DEBUG=1>)

target_compile_options(bzm PRIVATE -Wall -Wextra -Wpedantic)
if(BZM_WERROR)
  target_compile_options(bzm PRIVATE -Werror)
endif()

include(CTest)
if(BZM_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)

  # Optional Valgrind memcheck integration
  find_program(VALGRIND_EXECUTABLE valgrind)
  if(VALGRIND_EXECUTABLE)
    set(MEMORYCHECK_COMMAND "${VALGRIND_EXECUTABLE}")
    set(MEMORYCHECK_COMMAND_OPTIONS "--leak-check=full --show-leak-kinds=all --error-exitcode=1")
    set(CTEST_MEMORYCHECK_COMMAND "${VALGRIND_EXECUTABLE}")
    set(CTEST_MEMORYCHECK_TYPE "Valgrind")
    add_custom_target(memcheck
      COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -T memcheck
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "Running tests under Valgrind memcheck"
    )
  endif()
endif()

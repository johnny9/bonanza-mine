add_library(bzm_test_support STATIC
    mock_transport.c
)
target_link_libraries(bzm_test_support PRIVATE bzm)
target_include_directories(bzm_test_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Fetch Unity (no vendoring)
include(FetchContent)
option(BZM_FETCH_UNITY "Fetch Unity test framework" ON)
set(BZM_UNITY_TAG v2.6.0)

if(BZM_FETCH_UNITY)
  FetchContent_Declare(
    unity_src
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG        ${BZM_UNITY_TAG}
    GIT_SHALLOW    TRUE
  )
  # Use Unity's own CMake targets; do not redefine 'unity'.
  FetchContent_MakeAvailable(unity_src)
endif()

function(add_unity_test name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE bzm bzm_test_support unity)
  if(BZM_ENABLE_ASAN AND NOT MSVC)
    target_compile_options(${name} PRIVATE -fsanitize=address)
    target_link_options(${name} PRIVATE -fsanitize=address)
  endif()
  add_test(NAME ${name} COMMAND ${name})
endfunction()

add_unity_test(test_headers test_headers.c)
add_unity_test(test_commands test_commands.c)

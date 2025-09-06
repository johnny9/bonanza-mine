add_library(bzm_test_support STATIC
    mock_transport.c
)
target_link_libraries(bzm_test_support PRIVATE bzm)
target_include_directories(bzm_test_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

function(add_bzm_test name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE bzm bzm_test_support)
  if(BZM_ENABLE_ASAN AND NOT MSVC)
    target_compile_options(${name} PRIVATE -fsanitize=address)
    target_link_options(${name} PRIVATE -fsanitize=address)
  endif()
  add_test(NAME ${name} COMMAND ${name})
endfunction()

add_bzm_test(test_headers test_headers.c)
add_bzm_test(test_commands test_commands.c)


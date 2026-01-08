# Test targets

# Link test executables against clogging library
set(TEST_TARGETS
    test_basic_logging
)

foreach(test_target ${TEST_TARGETS})
    add_executable(${test_target} ${test_target}.c)
    target_link_libraries(${test_target} PRIVATE clogging)
    add_test(NAME ${test_target} COMMAND ${test_target})
endforeach()

# Platform-specific test for test_bench_basic_logging
if(WIN32)
    add_executable(test_bench_basic_logging test_bench_basic_logging_win32.c)
else()
    add_executable(test_bench_basic_logging test_bench_basic_logging_unix.c)
endif()

target_link_libraries(test_bench_basic_logging PRIVATE clogging)
add_test(NAME test_bench_basic_logging COMMAND test_bench_basic_logging)

# Platform-specific test for test_binary_logging
if(WIN32)
    add_executable(test_binary_logging test_binary_logging_win32.c)
    target_link_libraries(test_binary_logging PRIVATE clogging ws2_32)
else()
    add_executable(test_binary_logging test_binary_logging_unix.c)
    target_link_libraries(test_binary_logging PRIVATE clogging)
endif()

add_test(NAME test_binary_logging COMMAND test_binary_logging)

# Platform-specific test for test_bench_fd_logging
if(WIN32)
    add_executable(test_bench_fd_logging test_bench_fd_logging_win32.c)
    target_link_libraries(test_bench_fd_logging PRIVATE clogging ws2_32)
else()
    add_executable(test_bench_fd_logging test_bench_fd_logging_unix.c)
    target_link_libraries(test_bench_fd_logging PRIVATE clogging)
endif()

add_test(NAME test_bench_fd_logging COMMAND test_bench_fd_logging)

# Platform-specific test for test_fd_logging
if(WIN32)
    add_executable(test_fd_logging test_fd_logging_win32.c)
else()
    add_executable(test_fd_logging test_fd_logging_unix.c)
endif()

target_link_libraries(test_fd_logging PRIVATE clogging)
add_test(NAME test_fd_logging COMMAND test_fd_logging)

# Test targets

# Link test executables against clogging library
set(TEST_TARGETS
    test_basic_logging
    test_binary_logging
    test_bench_basic_logging
    test_bench_fd_logging
    test_fd_logging
)

foreach(test_target ${TEST_TARGETS})
    add_executable(${test_target} ${test_target}.c)
    target_link_libraries(${test_target} PRIVATE clogging)
    add_test(NAME ${test_target} COMMAND ${test_target})
endforeach()

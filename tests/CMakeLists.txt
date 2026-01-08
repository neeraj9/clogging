# Test targets

# Link test executables against clogging library
set(TEST_TARGETS
    test_basic_logging
)

foreach(test_target ${TEST_TARGETS})
    add_executable(${test_target} ${test_target}.c)
    target_link_libraries(${test_target} PRIVATE clogging)
    add_test(NAME ${test_target} COMMAND ${test_target})
    # Copy clogging DLL to test directory on Windows
    if(BUILD_SHARED_LIBS AND WIN32)
        add_custom_command(TARGET ${test_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:clogging>
            $<TARGET_FILE_DIR:${test_target}>)
    endif()
endforeach()

# UTF-8 tests (only if UTF-8 support is enabled)
if(CLOGGING_USE_UTF8_STRINGS)
    # Test UTF-8 validation functions
    add_executable(test_utf8_validation test_utf8_validation.c)
    target_link_libraries(test_utf8_validation PRIVATE clogging)
    add_test(NAME test_utf8_validation COMMAND test_utf8_validation)
    if(BUILD_SHARED_LIBS AND WIN32)
        add_custom_command(TARGET test_utf8_validation POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:clogging>
            $<TARGET_FILE_DIR:test_utf8_validation>)
    endif()
    
    # Test logging with UTF-8 strings
    add_executable(test_utf8_logging test_utf8_logging.c)
    target_link_libraries(test_utf8_logging PRIVATE clogging)
    add_test(NAME test_utf8_logging COMMAND test_utf8_logging)
    if(BUILD_SHARED_LIBS AND WIN32)
        add_custom_command(TARGET test_utf8_logging POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:clogging>
            $<TARGET_FILE_DIR:test_utf8_logging>)
    endif()
    
    # Test Windows UTF-16 to UTF-8 conversion (Windows only)
    if(WIN32)
        add_executable(test_utf8_conversion_win32 test_utf8_conversion_win32.c)
        target_link_libraries(test_utf8_conversion_win32 PRIVATE clogging)
        add_test(NAME test_utf8_conversion_win32 COMMAND test_utf8_conversion_win32)
        if(BUILD_SHARED_LIBS)
            add_custom_command(TARGET test_utf8_conversion_win32 POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:clogging>
                $<TARGET_FILE_DIR:test_utf8_conversion_win32>)
        endif()
    endif()
endif()

# Platform-specific test for test_bench_basic_logging
if(WIN32)
    add_executable(test_bench_basic_logging test_bench_basic_logging_win32.c)
else()
    add_executable(test_bench_basic_logging test_bench_basic_logging_unix.c)
endif()

target_link_libraries(test_bench_basic_logging PRIVATE clogging)
add_test(NAME test_bench_basic_logging COMMAND test_bench_basic_logging)

# Copy clogging DLL to test directory on Windows
if(BUILD_SHARED_LIBS AND WIN32)
    add_custom_command(TARGET test_bench_basic_logging POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:clogging>
        $<TARGET_FILE_DIR:test_bench_basic_logging>)
endif()

# Platform-specific test for test_binary_logging
if(WIN32)
    add_executable(test_binary_logging test_binary_logging_win32.c)
    target_link_libraries(test_binary_logging PRIVATE clogging ws2_32)
else()
    add_executable(test_binary_logging test_binary_logging_unix.c)
    target_link_libraries(test_binary_logging PRIVATE clogging)
endif()

add_test(NAME test_binary_logging COMMAND test_binary_logging)

# Copy clogging DLL to test directory on Windows
if(BUILD_SHARED_LIBS AND WIN32)
    add_custom_command(TARGET test_binary_logging POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:clogging>
        $<TARGET_FILE_DIR:test_binary_logging>)
endif()

# Platform-specific test for test_bench_fd_logging
if(WIN32)
    add_executable(test_bench_fd_logging test_bench_fd_logging_win32.c)
    target_link_libraries(test_bench_fd_logging PRIVATE clogging ws2_32)
else()
    add_executable(test_bench_fd_logging test_bench_fd_logging_unix.c)
    target_link_libraries(test_bench_fd_logging PRIVATE clogging)
endif()

add_test(NAME test_bench_fd_logging COMMAND test_bench_fd_logging)

# Copy clogging DLL to test directory on Windows
if(BUILD_SHARED_LIBS AND WIN32)
    add_custom_command(TARGET test_bench_fd_logging POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:clogging>
        $<TARGET_FILE_DIR:test_bench_fd_logging>)
endif()

# Platform-specific test for test_fd_logging
if(WIN32)
    add_executable(test_fd_logging test_fd_logging_win32.c)
else()
    add_executable(test_fd_logging test_fd_logging_unix.c)
endif()

target_link_libraries(test_fd_logging PRIVATE clogging)
add_test(NAME test_fd_logging COMMAND test_fd_logging)

# Copy clogging DLL to test directory on Windows
if(BUILD_SHARED_LIBS AND WIN32)
    add_custom_command(TARGET test_fd_logging POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:clogging>
        $<TARGET_FILE_DIR:test_fd_logging>)
endif()

# Basic Logging UTF-8 Example CMakeLists.txt

# Require minimum CMake version
cmake_minimum_required(VERSION 3.10)

# Create the executable for basic_logging_utf8_dynamic_lib example
add_executable(basic_logging_utf8_dynamic_lib basic_logging_utf8_dynamic_lib.c)

# Link against the dynamic clogging library
target_link_libraries(basic_logging_utf8_dynamic_lib PRIVATE clogging)

# Copy shared library to the executable directory for runtime
add_custom_command(TARGET basic_logging_utf8_dynamic_lib POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:clogging>
    $<TARGET_FILE_DIR:basic_logging_utf8_dynamic_lib>/$<TARGET_FILE_NAME:clogging>
    COMMENT "Copying clogging shared library to executable directory"
)

# Include directories
target_include_directories(basic_logging_utf8_dynamic_lib PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Apply UTF-8 compilation flags if enabled
if(CLOGGING_USE_UTF8_STRINGS)
    if(MSVC)
        target_compile_options(basic_logging_utf8_dynamic_lib PRIVATE /utf-8)
    elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(basic_logging_utf8_dynamic_lib PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
    endif()
    target_compile_definitions(basic_logging_utf8_dynamic_lib PRIVATE CLOGGING_USE_UTF8_STRINGS)
endif()

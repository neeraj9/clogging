# Basic Logging UTF-8 Example CMakeLists.txt

# Require minimum CMake version
cmake_minimum_required(VERSION 3.10)

# Create the executable for basic_logging_utf8_multi_thread example
if(WIN32)
    add_executable(basic_logging_utf8_multi_thread basic_logging_utf8_multi_thread_win32.c)
else()
    add_executable(basic_logging_utf8_multi_thread basic_logging_utf8_multi_thread_unix.c)
endif()

# Link against the static clogging library
target_link_libraries(basic_logging_utf8_multi_thread PRIVATE clogging_static)

# Include directories
target_include_directories(basic_logging_utf8_multi_thread PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Apply UTF-8 compilation flags if enabled
if(CLOGGING_USE_UTF8_STRINGS)
    if(MSVC)
        target_compile_options(basic_logging_utf8_multi_thread PRIVATE /utf-8)
    elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(basic_logging_utf8_multi_thread PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
    endif()
    target_compile_definitions(basic_logging_utf8_multi_thread PRIVATE CLOGGING_USE_UTF8_STRINGS)
endif()

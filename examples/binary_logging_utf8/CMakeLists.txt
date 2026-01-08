# Binary Logging UTF-8 Example CMakeLists.txt

# Require minimum CMake version
cmake_minimum_required(VERSION 3.10)

# Create the executable for binary_logging_utf8 example
add_executable(binary_logging_utf8 binary_logging_utf8.c)

# Link against the static clogging library
target_link_libraries(binary_logging_utf8 PRIVATE clogging_static)

# Include directories
target_include_directories(binary_logging_utf8 PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Apply UTF-8 compilation flags if enabled
if(CLOGGING_USE_UTF8_STRINGS)
    if(MSVC)
        target_compile_options(binary_logging_utf8 PRIVATE /utf-8)
    elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(binary_logging_utf8 PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
    endif()
    target_compile_definitions(binary_logging_utf8 PRIVATE CLOGGING_USE_UTF8_STRINGS)
endif()

# Basic Logging UTF-8 Example CMakeLists.txt

# Require minimum CMake version
cmake_minimum_required(VERSION 3.10)

# Create the executable for basic_logging_utf8_json example
add_executable(basic_logging_utf8_json basic_logging_utf8_json.c)

# Link against the static clogging library
target_link_libraries(basic_logging_utf8_json PRIVATE clogging_static)

# Include directories
target_include_directories(basic_logging_utf8_json PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Apply UTF-8 compilation flags if enabled
if(CLOGGING_USE_UTF8_STRINGS)
    if(MSVC)
        target_compile_options(basic_logging_utf8_json PRIVATE /utf-8)
    elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(basic_logging_utf8_json PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
    endif()
    target_compile_definitions(basic_logging_utf8_json PRIVATE CLOGGING_USE_UTF8_STRINGS)
endif()

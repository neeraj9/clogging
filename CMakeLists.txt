cmake_minimum_required(VERSION 3.10)
project(clogging VERSION 0.1 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)  # Enable gnu99 extensions

# Options
option(BUILD_TESTS "Build test executables" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Find required packages
find_package(Threads REQUIRED)

# Check for required headers (optional on Windows with native threading)
include(CheckIncludeFile)
if(NOT WIN32)
    check_include_file(pthread.h HAVE_PTHREAD_H)
    if(NOT HAVE_PTHREAD_H)
        message(FATAL_ERROR "pthread.h is required but not found")
    endif()
endif()

# Check for optional libraries
include(CheckLibraryExists)
check_library_exists(rt gmtime_r "time.h" HAVE_RT_LIB)

# Check for gmtime_r on the current system
include(CheckSymbolExists)
set(CMAKE_REQUIRED_DEFINITIONS_SAVE ${CMAKE_REQUIRED_DEFINITIONS})
# set(CMAKE_REQUIRED_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS} -D_POSIX_C_SOURCE=200809L")
check_symbol_exists(gmtime_r "time.h" HAVE_GMTIME_R)
set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS_SAVE})
if(HAVE_GMTIME_R)
    add_compile_definitions(HAVE_GMTIME_R)
endif()

# Set compile flags (compiler-specific)
if(MSVC)
    add_compile_options(/W4)  # Wall equivalent for MSVC
else()
    add_compile_options(-Wall -Wextra)
endif()

# Add subdirectories
add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation of CMake config files
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/cloggingConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cloggingConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cloggingConfig.cmake"
    INSTALL_DESTINATION lib/cmake/clogging
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/cloggingConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/cloggingConfigVersion.cmake"
    DESTINATION lib/cmake/clogging
)

install(
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cloggingTargets.cmake"
    DESTINATION lib/cmake/clogging
)

# Install pkg-config file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/clogging.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/clogging.pc"
    @ONLY
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/clogging.pc"
    DESTINATION lib/pkgconfig
)

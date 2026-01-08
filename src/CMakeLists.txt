# Source library targets

# Create the main clogging library
add_library(clogging
    basic_logging.c
    binary_logging.c
    fd_logging.c
    logging_common.c
)

# Add UTF-8 utilities if UTF-8 support is enabled
if(CLOGGING_USE_UTF8_STRINGS)
    target_sources(clogging PRIVATE utf8_utils.c)
endif()

# Set public headers
target_include_directories(clogging PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/clogging>
)

# Link against required libraries
target_link_libraries(clogging PUBLIC Threads::Threads)

# Link against rt library if available (non-Windows)
if(HAVE_RT_LIB)
    target_link_libraries(clogging PRIVATE rt)
endif()

# Link against Windows libraries if on Windows
if(WIN32)
    target_link_libraries(clogging PRIVATE ws2_32)
endif()

# Set version
set_target_properties(clogging PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)

# Installation
install(TARGETS clogging
    EXPORT cloggingTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES
    basic_logging.h
    binary_logging.h
    fd_logging.h
    logging_common.h
    DESTINATION include/clogging
)

# Install UTF-8 utilities header if UTF-8 support is enabled
if(CLOGGING_USE_UTF8_STRINGS)
    install(FILES utf8_utils.h DESTINATION include/clogging)
endif()

# Create and install targets file
install(EXPORT cloggingTargets
    FILE cloggingTargets.cmake
    NAMESPACE clogging::
    DESTINATION lib/cmake/clogging
)

# Generate and install a header with export declarations (optional, for better symbol visibility)
# This is useful for Windows DLL exports
if(WIN32)
    set_target_properties(clogging PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

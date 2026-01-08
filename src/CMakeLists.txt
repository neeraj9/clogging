# Source library targets

# Create the main clogging library
add_library(clogging
    basic_logging.c
    binary_logging.c
    fd_logging.c
    logging_common.c
)

# Set public headers
target_include_directories(clogging PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/clogging>
)

# Link against required libraries
target_link_libraries(clogging PUBLIC Threads::Threads)

# Link against rt library if available
if(HAVE_RT_LIB)
    target_link_libraries(clogging PRIVATE rt)
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

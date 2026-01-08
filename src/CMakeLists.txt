# Source library targets

# Create the main clogging library (shared or static based on BUILD_SHARED_LIBS)
add_library(clogging
    basic_logging.c
    binary_logging.c
    fd_logging.c
    logging_common.c
)

# Create static library if BUILD_STATIC_LIBS is ON
if(BUILD_STATIC_LIBS)
    add_library(clogging_static STATIC
        basic_logging.c
        binary_logging.c
        fd_logging.c
        logging_common.c
    )
endif()

# Add UTF-8 utilities if UTF-8 support is enabled
if(CLOGGING_USE_UTF8_STRINGS)
    target_sources(clogging PRIVATE utf8_utils.c)
    if(BUILD_STATIC_LIBS)
        target_sources(clogging_static PRIVATE utf8_utils.c)
    endif()
endif()

# Set public headers
target_include_directories(clogging PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/clogging>
)

if(BUILD_STATIC_LIBS)
    target_include_directories(clogging_static PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/clogging>
    )
endif()

# Link against required libraries
target_link_libraries(clogging PUBLIC Threads::Threads)

if(BUILD_STATIC_LIBS)
    target_link_libraries(clogging_static PUBLIC Threads::Threads)
endif()

# Link against rt library if available (non-Windows)
if(HAVE_RT_LIB)
    target_link_libraries(clogging PRIVATE rt)
    if(BUILD_STATIC_LIBS)
        target_link_libraries(clogging_static PRIVATE rt)
    endif()
endif()

# Link against Windows libraries if on Windows
if(WIN32)
    target_link_libraries(clogging PRIVATE ws2_32)
    if(BUILD_STATIC_LIBS)
        target_link_libraries(clogging_static PRIVATE ws2_32)
    endif()
endif()

# Set version
set_target_properties(clogging PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)

if(BUILD_STATIC_LIBS)
    set_target_properties(clogging_static PROPERTIES
        VERSION ${PROJECT_VERSION}
    )
endif()

# Installation
install(TARGETS clogging
    EXPORT cloggingTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

if(BUILD_STATIC_LIBS)
    install(TARGETS clogging_static
        EXPORT cloggingTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
    )
endif()

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

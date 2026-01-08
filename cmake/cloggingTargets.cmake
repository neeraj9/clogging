# This file will be replaced by the generated targets file during installation
# It is included here as a placeholder for the CMake configuration

if(NOT TARGET clogging::clogging)
    add_library(clogging::clogging UNKNOWN IMPORTED)
    
    set_target_properties(clogging::clogging PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/../../../lib/libclogging.a"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../../../include/clogging"
        INTERFACE_LINK_LIBRARIES "Threads::Threads"
    )
endif()

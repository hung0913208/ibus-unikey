# Try to find IBus
#
# once done this will define
# IBUS_FOUND - System has libfuse
# IBUS_INCLUDE_DIRS - The libfuse include directories
# IBUS_LIBRARIES - The libraries needed to use libfuse
# IBUS_DEFINITIONS - Compiler switches required for using libfuse

### DEBUGGING ###
# to activate run cmake ../src -DCMAKE_MODULES_DEBUG

function(ibusdebug _varname)
    if(CMAKE_MODULES_DEBUG)
        message("${_varname} = ${${_varname}}")
    endif(CMAKE_MODULES_DEBUG)
endfunction(ibusdebug)

set(IBUS_FOUND FALSE)

# did we have pkg-config?
find_package(PkgConfig)

# using pkg-config for ibus
if (PkgConfig_FOUND)
    pkg_check_modules(PC_IBUS ibus-1.0)

    if(PC_IBUS_FOUND)
        ibusdebug(PC_IBUS_LIBRARIES)
        ibusdebug(PC_IBUS_LIBRARY_DIRS)
        ibusdebug(PC_IBUS_LDFLAGS)
        ibusdebug(PC_IBUS_LDFLAGS_OTHER)
        ibusdebug(PC_IBUS_INCLUDE_DIRS)
        ibusdebug(PC_IBUS_CFLAGS)
        ibusdebug(PC_IBUS_CFLAGS_OTHER)

        # search for headers
        find_path(IBUS_INCLUDE_DIR ibus.h HINTS ${PC_IBUS_INCLUDEDIR}
                  ${PC_IBUS_INCLUDE_DIRS} PATH_SUFFIXES ibus libibus)

        # search for libs
        find_library(IBUS_LIBRARY NAMES ibus-1.0
                     PATH ${PC_IBUS_LIBDIR} ${PC_IBUS_LIBRARY_DIRS}
                     PATH_SUFFIXES ibus-1.0/)
        set(IBUS_FOUND TRUE)
    endif()
else()
    # search for headers
    find_path(IBUS_INCLUDE_DIR ibus.h PATH_SUFFIXES ibus-1.0/)
    if (NOT IBUS_INCLUDE_DIR)
        message(FATAL_ERROR "ibus directory not found")
    endif()

    # search for libs
    find_library(IBUS_LIBRARY NAMES ibus libibus PATH_SUFFIXES ibus-1.0/)
    if (NOT IBUS_LIBRARY)
        message(FATAL_ERROR "ibus libraries not found")
    endif()

    set(IBUS_FOUND TRUE)

    ibusdebug(IBUS_INCLUDE_DIR)
    ibusdebug(IBUS_LIBRARY)
endif()

# set IBUS_DEFINITIONS to cflags
set(IBUS_DEFINITIONS ${PC_IBUS_CFLAGS_OTHER} )

# set headers
set(IBUS_INCLUDE_DIRS ${IBUS_INCLUDE_DIR})

# set libs
set(IBUS_LIBRARIES ${IBUS_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(IBUS IBUS_LIBRARY IBUS_INCLUDE_DIR)
mark_as_advanced(IBUS_LIBRARY IBUS_INCLUDE_DIR)
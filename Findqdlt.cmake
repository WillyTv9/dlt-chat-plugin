# Findqdlt.cmake - Find DLT Viewer qdlt SDK
#
# This module finds the qdlt library installed by dlt-viewer.
# It creates an imported target `qdlt` that can be linked against.
#
# Variables:
#   QDLT_FOUND        - True if qdlt was found
#   QDLT_INCLUDE_DIR  - Include directory containing qdlt headers
#   QDLT_LIBRARY      - Path to qdlt import library (.lib)
#   QDLT_RUNTIME      - Path to qdlt runtime DLL
#
# The module checks these locations in order:
#   1. QDLT_ROOT environment variable
#   2. DLT_VIEWER_ROOT environment variable
#   3. CMAKE_INSTALL_PREFIX
#   4. Default install: $LOCALAPPDATA/Programs/dlt-viewer (Windows)
#   5. Default install: /usr/local (Unix)

if(TARGET qdlt)
    set(QDLT_FOUND TRUE)
    return()
endif()

# Candidate base directories
set(_qdlt_candidates)

if(DEFINED ENV{QDLT_ROOT})
    list(APPEND _qdlt_candidates "$ENV{QDLT_ROOT}")
endif()
if(DEFINED ENV{DLT_VIEWER_ROOT})
    list(APPEND _qdlt_candidates "$ENV{DLT_VIEWER_ROOT}")
endif()
list(APPEND _qdlt_candidates "${CMAKE_INSTALL_PREFIX}")

if(WIN32)
    file(TO_CMAKE_PATH "$ENV{LOCALAPPDATA}" _localappdata)
    list(APPEND _qdlt_candidates "${_localappdata}/Programs/dlt-viewer/sdk")
endif()
if(UNIX)
    list(APPEND _qdlt_candidates "/usr/local")
endif()

set(QDLT_FOUND FALSE)

foreach(_base IN LISTS _qdlt_candidates)
    set(_inc "${_base}/include/qdlt")
    if(MINGW)
        set(_lib "${_base}/lib/libqdlt_mingw.dll.a")
        if(NOT EXISTS "${_lib}")
            set(_lib "${_base}/lib/libqdlt.dll.a")
        endif()
    else()
        set(_lib "${_base}/lib/qdlt.lib")
    endif()
    if(MINGW)
        set(_dll "${_base}/lib/libqdlt_mingw.dll")
    else()
        set(_dll "${_base}/qdlt.dll")
    endif()

    if(EXISTS "${_inc}/qdlt.h" AND EXISTS "${_lib}")
        set(QDLT_INCLUDE_DIR "${_inc}" CACHE PATH "qdlt include directory")
        set(QDLT_LIBRARY "${_lib}" CACHE FILEPATH "qdlt import library")
        set(QDLT_RUNTIME "${_dll}" CACHE FILEPATH "qdlt runtime DLL")
        set(QDLT_FOUND TRUE)
        break()
    endif()
endforeach()

if(NOT QDLT_FOUND)
    # Try finding via CMake's find_path/find_library as fallback
    find_path(QDLT_INCLUDE_DIR qdlt.h
        PATHS ENV QDLT_ROOT ENV DLT_VIEWER_ROOT
        PATH_SUFFIXES sdk/include/qdlt include/qdlt
    )
    find_library(QDLT_LIBRARY qdlt
        PATHS ENV QDLT_ROOT ENV DLT_VIEWER_ROOT
        PATH_SUFFIXES sdk/lib lib
    )
    if(QDLT_INCLUDE_DIR AND QDLT_LIBRARY)
        set(QDLT_FOUND TRUE)
        if(NOT QDLT_RUNTIME)
            set(QDLT_RUNTIME "${QDLT_LIBRARY}")
        endif()
    endif()
endif()

if(QDLT_FOUND AND NOT TARGET qdlt)
    add_library(qdlt SHARED IMPORTED)
    set_target_properties(qdlt PROPERTIES
        IMPORTED_IMPLIB "${QDLT_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${QDLT_INCLUDE_DIR}"
    )
    if(QDLT_RUNTIME AND EXISTS "${QDLT_RUNTIME}")
        set_target_properties(qdlt PROPERTIES
            IMPORTED_LOCATION "${QDLT_RUNTIME}"
        )
    endif()
    if(MINGW)
        set_target_properties(qdlt PROPERTIES
            IMPORTED_IMPLIB "${QDLT_LIBRARY}"
        )
    endif()

    # Mark as found and cached
    mark_as_advanced(QDLT_INCLUDE_DIR QDLT_LIBRARY QDLT_RUNTIME)
endif()

if(QDLT_FOUND)
    if(NOT QDLT_FIND_QUIETLY)
        message(STATUS "Found qdlt: ${QDLT_LIBRARY}")
    endif()
else()
    if(QDLT_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find qdlt (dlt-viewer SDK). "
            "Install dlt-viewer or set QDLT_ROOT environment variable.")
    endif()
endif()

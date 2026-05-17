cmake_minimum_required(VERSION 3.16)
project(dltchatplugin
    VERSION 0.7.0
    DESCRIPTION "Chat Log Assistant plugin for DLT Viewer"
    LANGUAGES CXX
)

# ── C++ Standard ──────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Qt Framework ──────────────────────────────────────────────────
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED QT_PREFIX)
    find_package(Qt6 QUIET COMPONENTS Core Gui Widgets Network Xml Concurrent)
    if(Qt6_FOUND)
        set(QT_PREFIX Qt6)
    else()
        find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network Xml)
        set(QT_PREFIX Qt5)
    endif()
else()
    find_package(${QT_PREFIX} REQUIRED COMPONENTS Core Gui Widgets Network Xml Concurrent)
endif()

# ── DLT Viewer SDK ────────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
find_package(qdlt REQUIRED)

# ── Compiler Warnings ─────────────────────────────────────────────
include(cmake/compiler_warnings.cmake)

# ── Sanitizers (Debug only) ──────────────────────────────────────
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT MSVC)
    option(DLT_ENABLE_ASAN "Enable AddressSanitizer" OFF)
    if(DLT_ENABLE_ASAN)
        add_compile_options(-fsanitize=address -fsanitize=undefined)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()
endif()

# ── Versioned Build ───────────────────────────────────────────────
include(cmake/GetGitHash.cmake)
get_git_hash(DLTCHAT_GIT_HASH)
string(TIMESTAMP DLTCHAT_BUILD_TIMESTAMP UTC)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/dltchat_version.h.in"
    "${CMAKE_BINARY_DIR}/generated/dltchat_version.h"
    @ONLY
)

# ── Source Tree ───────────────────────────────────────────────────
add_subdirectory(src)

# ── Clang-Tidy ─────────────────────────────────────────────────────
find_program(CLANG_TIDY clang-tidy)
if(CLANG_TIDY)
    if(TARGET dltchatplugin)
        set_target_properties(dltchatplugin PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY};--config-file=${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy"
        )
    endif()
    if(TARGET dltchat_app_logic)
        set_target_properties(dltchat_app_logic PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY};--config-file=${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy"
        )
    endif()
    message(STATUS "clang-tidy found: ${CLANG_TIDY}")
else()
    message(STATUS "clang-tidy not found — skipping static analysis")
endif()

# ── Warnings ──────────────────────────────────────────────────────
if(TARGET dltchatplugin)
    dltchat_set_warnings(dltchatplugin)
endif()
if(TARGET dltchat_app_logic)
    dltchat_set_warnings(dltchat_app_logic)
endif()

# ── Tests (optional) ─────────────────────────────────────────────
option(DLTCHAT_BUILD_TESTS "Build unit tests" OFF)
if(DLTCHAT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── Packaging (optional) ──────────────────────────────────────────
option(DLTCHAT_BUILD_DIST "Build distribution package" OFF)
if(DLTCHAT_BUILD_DIST)
    add_subdirectory(dist)
endif()

# ── Audit tools (optional) ────────────────────────────────────────
option(DLTCHAT_BUILD_TOOLS "Build audit tools (requires qdlt SDK)" OFF)
if(DLTCHAT_BUILD_TOOLS)
    add_subdirectory(tools/quick_action_audit)
endif()

# Helper macro for consistent dependency fetching
#
# Usage:
#   dltchat_fetch_dependency(
#     NAME      nlohmann_json
#     GIT_REPO  https://github.com/nlohmann/json.git
#     GIT_TAG   v3.11.3
#   )
#
# The fetched target is available as ${NAME}::${NAME} or ${NAME}.

macro(dltchat_fetch_dependency)
    set(options "")
    set(oneValueArgs NAME GIT_REPO GIT_TAG)
    set(multiValueArgs "")
    cmake_parse_arguments(FETCH "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT FETCH_NAME OR NOT FETCH_GIT_REPO OR NOT FETCH_GIT_TAG)
        message(FATAL_ERROR "dltchat_fetch_dependency requires NAME, GIT_REPO, and GIT_TAG")
    endif()

    include(FetchContent)
    FetchContent_Declare(${FETCH_NAME}
        GIT_REPOSITORY ${FETCH_GIT_REPO}
        GIT_TAG        ${FETCH_GIT_TAG}
        GIT_SHALLOW    TRUE
        SOURCE_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/_deps/${FETCH_NAME}"
        BINARY_DIR     "${CMAKE_CURRENT_BINARY_DIR}/_deps/${FETCH_NAME}-build"
    )
    FetchContent_MakeAvailable(${FETCH_NAME})

    message(STATUS "vendor: ${FETCH_NAME} @ ${FETCH_GIT_TAG}")
endmacro()

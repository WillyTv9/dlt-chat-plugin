function(get_git_hash var)
    set(git_hash "unknown")
    find_package(Git QUIET)
    if(Git_FOUND)
        execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%H
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE git_hash
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
    endif()
    set(${var} "${git_hash}" PARENT_SCOPE)
endfunction()

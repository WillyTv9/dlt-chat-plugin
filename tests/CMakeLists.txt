find_package(${QT_PREFIX} REQUIRED COMPONENTS Test)

set(TEST_SOURCES
    test_main.cpp
    test_rulebasedanalyzer.cpp
    test_automotivelogparser.cpp
    test_contextualextractor.cpp
    test_conversationmanager.cpp
    test_fibexenricher.cpp
    test_temporalcorrelator.cpp
    test_llmutils.cpp
    test_dltexport.cpp
    test_userfiltermanager.cpp
)

add_executable(tests ${TEST_SOURCES})

target_include_directories(tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/app_logic/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(tests
    PRIVATE
        dltchat_app_logic
        ${QT_PREFIX}::Core
        ${QT_PREFIX}::Gui
        ${QT_PREFIX}::Network
        ${QT_PREFIX}::Test
        ${QT_PREFIX}::Xml
)

# Resolve Qt bin dir so ctest can load Qt DLLs (Windows STATUS_DLL_NOT_FOUND / 0xc0000135).
set(_qt_bin_dir "")
if(TARGET ${QT_PREFIX}::Core)
    foreach(_cfg RELEASE RELWITHDEBINFO MINSIZEREL DEBUG)
        get_target_property(_qt_core ${QT_PREFIX}::Core IMPORTED_LOCATION_${_cfg})
        if(_qt_core)
            get_filename_component(_qt_bin_dir "${_qt_core}" DIRECTORY)
            break()
        endif()
    endforeach()
    if(NOT _qt_bin_dir)
        get_target_property(_qt_core ${QT_PREFIX}::Core IMPORTED_LOCATION)
        if(_qt_core)
            get_filename_component(_qt_bin_dir "${_qt_core}" DIRECTORY)
        endif()
    endif()
endif()
if(NOT _qt_bin_dir AND DEFINED ENV{QT_ROOT_DIR})
    set(_qt_bin_dir "$ENV{QT_ROOT_DIR}/bin")
endif()

set(_test_prefix_dirs)
if(_qt_bin_dir)
    list(APPEND _test_prefix_dirs "${_qt_bin_dir}")
endif()
if(CMAKE_CXX_COMPILER)
    get_filename_component(_compiler_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
    list(APPEND _test_prefix_dirs "${_compiler_bin}")
endif()
if(CMAKE_PREFIX_PATH)
    foreach(_prefix IN LISTS CMAKE_PREFIX_PATH)
        list(APPEND _test_prefix_dirs "${_prefix}/bin")
    endforeach()
endif()
if(DEFINED ENV{QDLT_ROOT})
    list(APPEND _test_prefix_dirs "$ENV{QDLT_ROOT}" "$ENV{QDLT_ROOT}/lib")
endif()
list(REMOVE_DUPLICATES _test_prefix_dirs)

set(_qt_plugin_path "")
if(DEFINED ENV{QT_PLUGIN_PATH})
    set(_qt_plugin_path "$ENV{QT_PLUGIN_PATH}")
elseif(_qt_bin_dir)
    get_filename_component(_qt_root "${_qt_bin_dir}" DIRECTORY)
    set(_qt_plugin_path "${_qt_root}/plugins")
endif()

if(MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
        OUTPUT_VARIABLE _gcc_version
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(_gcc_version VERSION_LESS "11.0" OR _gcc_version VERSION_GREATER_EQUAL "12.0")
        message(WARNING
            "MinGW GCC ${_gcc_version} is likely incompatible with Qt at ${_qt_bin_dir} "
            "(official Qt 6.7 MinGW builds use GCC 11.2). Unit tests may crash at runtime. "
            "Install MinGW 11.2 from the Qt installer or build with MSVC.")
    endif()
endif()

find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
if(WIN32 AND WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET tests POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
            --no-translations --no-system-d3d-compiler --no-opengl-sw
            "$<TARGET_FILE:tests>"
        COMMENT "Deploying Qt runtime for unit tests"
    )
endif()

if(WIN32)
    # Run from the directory that contains tests.exe (and windeployqt output).
    add_test(NAME tests
        COMMAND ${CMAKE_COMMAND} -E chdir $<TARGET_FILE_DIR:tests> $<TARGET_FILE:tests>)
else()
    list(JOIN _test_prefix_dirs ":" _test_path_prefix)
    if(_test_path_prefix AND DEFINED ENV{PATH})
        set(_test_path_env "${_test_path_prefix}:$ENV{PATH}")
    elseif(_test_path_prefix)
        set(_test_path_env "${_test_path_prefix}")
    else()
        set(_test_path_env "$ENV{PATH}")
    endif()
    add_test(NAME tests COMMAND $<TARGET_FILE:tests>)
    if(_qt_plugin_path)
        set_tests_properties(tests PROPERTIES
            ENVIRONMENT "PATH=${_test_path_env};QT_PLUGIN_PATH=${_qt_plugin_path}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )
    else()
        set_tests_properties(tests PROPERTIES
            ENVIRONMENT "PATH=${_test_path_env}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )
    endif()
endif()

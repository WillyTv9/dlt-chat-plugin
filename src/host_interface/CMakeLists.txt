add_library(dltchatplugin MODULE
    plugin_entry.cpp
    chatform.cpp
    results_model.cpp
    dltaioptionsdialog.cpp
)

target_include_directories(dltchatplugin
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
)


target_link_libraries(dltchatplugin
    PRIVATE
        dltchat_app_logic
        qdlt
    PUBLIC
        ${QT_PREFIX}::Gui
        ${QT_PREFIX}::Widgets
        ${QT_PREFIX}::Network
        ${QT_PREFIX}::Xml
)

target_compile_definitions(dltchatplugin PRIVATE
    PLUGIN_INTERFACE_VERSION="1.0.1"
    DLT_CHAT_PLUGIN_VERSION="${PROJECT_VERSION}"
)

target_include_directories(dltchatplugin
    PRIVATE "${CMAKE_BINARY_DIR}/generated"
)

set_target_properties(dltchatplugin PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PREFIX ""
)

if(MSVC)
    # Suppress C4458 from DLT Viewer SDK headers (parameter hides class member)
    # /source-charset:utf-8 needed for Qt5/MSVC: source files contain Unicode literals
    # that MSVC misinterprets under the default Windows-1252 code page (C4566).
    target_compile_options(dltchatplugin PRIVATE /wd4458 /source-charset:utf-8)
else()
    target_compile_options(dltchatplugin PRIVATE -fvisibility=hidden)
endif()

if(COMMAND add_plugin)
    add_plugin(dltchatplugin)
endif()

# Detect QDltFile::setManualMarkerIndices availability (added in DLT Viewer ~2.30)
get_target_property(_qdlt_inc qdlt INTERFACE_INCLUDE_DIRECTORIES)
if(EXISTS "${_qdlt_inc}/qdltfile.h")
    file(READ "${_qdlt_inc}/qdltfile.h" _qdlt_file_h)
    string(FIND "${_qdlt_file_h}" "setManualMarkerIndices" _has_manual_markers)
    if(NOT _has_manual_markers EQUAL -1)
        target_compile_definitions(dltchatplugin PRIVATE QDLT_HAS_SET_MANUAL_MARKER_INDICES=1)
    endif()
endif()


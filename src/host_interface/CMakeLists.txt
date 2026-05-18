add_library(dltchatplugin MODULE
    plugin_entry.cpp
    chatform.cpp
    results_model.cpp
    dltaioptionsdialog.cpp
)

target_include_directories(dltchatplugin
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_sources(dltchatplugin PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../resources/dltchat_resources.qrc"
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
    target_compile_options(dltchatplugin PRIVATE /wd4458)
else()
    target_compile_options(dltchatplugin PRIVATE -fvisibility=hidden)
endif()

if(COMMAND add_plugin)
    add_plugin(dltchatplugin)
endif()


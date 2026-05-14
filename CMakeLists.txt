cmake_minimum_required(VERSION 3.16)
project(dltchatplugin
    VERSION 0.5.0
    DESCRIPTION "Chat Log Assistant plugin for DLT Viewer"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED QT_PREFIX)
    find_package(Qt6 QUIET COMPONENTS Core Gui Widgets Network Xml)
    if(Qt6_FOUND)
        set(QT_PREFIX Qt6)
    else()
        find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network Xml)
        set(QT_PREFIX Qt5)
    endif()
else()
    find_package(${QT_PREFIX} REQUIRED COMPONENTS Core Gui Widgets Network Xml)
endif()

# Find qdlt (DLT Viewer SDK) - must be before target_link_libraries
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
find_package(qdlt REQUIRED)

add_library(dltchatplugin MODULE
    dltchatplugin.cpp
    chatform.cpp
    contextualextractor.cpp
    conversationmanager.cpp
    fibexenricher.cpp
    temporalcorrelator.cpp
    dltexport.cpp
    dltanalyzerinterface.cpp
    dltllmanalyzerinterface.cpp
    dltbulkanalyzer.cpp
    dltaioptionsdialog.cpp
    automotivelogparser.cpp
    userfiltermanager.cpp
)

target_compile_definitions(dltchatplugin PRIVATE
    PLUGIN_INTERFACE_VERSION="1.0.1"
)

target_link_libraries(dltchatplugin
    qdlt
    ${QT_PREFIX}::Gui
    ${QT_PREFIX}::Widgets
    ${QT_PREFIX}::Network
    ${QT_PREFIX}::Xml
)

set_target_properties(dltchatplugin PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

if(COMMAND add_plugin)
    add_plugin(dltchatplugin)
endif()

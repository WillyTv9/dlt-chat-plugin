cmake_minimum_required(VERSION 3.16)
project(dltchatplugin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED QT_PREFIX)
    find_package(Qt6 QUIET COMPONENTS Core Gui Widgets Network)
    if(Qt6_FOUND)
        set(QT_PREFIX Qt6)
    else()
        find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network)
        set(QT_PREFIX Qt5)
    endif()
else()
    find_package(${QT_PREFIX} REQUIRED COMPONENTS Core Gui Widgets Network)
endif()

add_library(dltchatplugin MODULE
    dltchatplugin.cpp
    chatform.cpp
    dltexport.cpp
    dltanalyzerinterface.cpp
    dltllmanalyzerinterface.cpp
    dltaioptionsdialog.cpp
)

target_compile_definitions(dltchatplugin PRIVATE
    PLUGIN_INTERFACE_VERSION="1.0.1"
)

target_link_libraries(dltchatplugin
    qdlt
    ${QT_PREFIX}::Gui
    ${QT_PREFIX}::Widgets
    ${QT_PREFIX}::Network
)

if(COMMAND add_plugin)
    add_plugin(dltchatplugin)
endif()

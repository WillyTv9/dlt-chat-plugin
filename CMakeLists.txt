cmake_minimum_required(VERSION 3.16)
project(dltchatplugin)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable AUTOMOC, AUTORCC, and AUTOUIC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Define the DLT Viewer SDK Path
# Change this if the path is different
set(DLT_VIEWER_SDK_PATH "C:/Users/andrea.franco/Desktop/DLTViewer/DLTViewer-2.30.0-STABLE-qt6.8.3-r1318_msvc2022_x64-win64")

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

# Add DLT Viewer include directories
include_directories("${DLT_VIEWER_SDK_PATH}/sdk/include/qdlt")

# Define the plugin library
add_library(dltchatplugin MODULE
    dltchatplugin.cpp
    dltchatanalyzer.cpp
    chatform.cpp
    dltexport.cpp
    dltanalyzerinterface.cpp
    dltllmanalyzerinterface.cpp
)

# Link to DLT Viewer library (qdlt.lib) and Qt libraries
target_link_libraries(dltchatplugin
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    "${DLT_VIEWER_SDK_PATH}/sdk/lib/qdlt.lib"
)

# Define version and metadata
target_compile_definitions(dltchatplugin PRIVATE
    PLUGIN_INTERFACE_VERSION="2.1.0"
)

# Set output name to dltchatplugin.dll
set_target_properties(dltchatplugin PROPERTIES
    PREFIX ""
    SUFFIX ".dll"
)
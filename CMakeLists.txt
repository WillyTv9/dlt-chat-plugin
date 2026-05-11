# Copyright (C) 2026
#
# This file is part of COVESA DLT-Viewer project.
#
# This Source Code Form is subject to the terms of the
# Mozilla Public License (MPL), v. 2.0.
# If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.
#
# DLT Log Assistant plugin - Enhanced version with CSV export and LLM support

add_library(dltchatplugin MODULE
    dltchatplugin.cpp
    dltchatanalyzer.cpp
    chatform.cpp
    dltexport.cpp
    dltanalyzerinterface.cpp
    dltllmanalyzerinterface.cpp)

target_link_libraries(dltchatplugin qdlt ${QT_PREFIX}::Widgets ${QT_PREFIX}::Network)

add_plugin(dltchatplugin)

add_executable(dltchatplugin_test
    dltchatplugin_test.cpp
    dltchatanalyzer.cpp
    dltexport.cpp
    dltanalyzerinterface.cpp)

target_link_libraries(dltchatplugin_test ${QT_PREFIX}::Core)
target_include_directories(dltchatplugin_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

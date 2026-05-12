cmake_minimum_required(VERSION 3.16)
project(dltchatplugin_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 QUIET COMPONENTS Core Test)
if(NOT Qt5_FOUND)
    find_package(Qt6 REQUIRED COMPONENTS Core Test)
    set(QT_PREFIX Qt6)
else()
    set(QT_PREFIX Qt5)
endif()

set(SOURCES_BASE
    ${CMAKE_SOURCE_DIR}/../dltanalyzerinterface.cpp
    ${CMAKE_SOURCE_DIR}/../automotivelogparser.cpp
    ${CMAKE_SOURCE_DIR}/../dltexport.cpp
    ${CMAKE_SOURCE_DIR}/../userfiltermanager.cpp
)

set(TEST_SOURCES
    test_main.cpp
    test_rulebasedanalyzer.cpp
    test_automotivelogparser.cpp
    test_llmutils.cpp
    test_dltexport.cpp
    test_userfiltermanager.cpp
)

add_executable(tests
    ${TEST_SOURCES}
    ${SOURCES_BASE}
)

target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/..)
target_link_libraries(tests ${QT_PREFIX}::Core ${QT_PREFIX}::Test)

add_test(NAME tests COMMAND tests)

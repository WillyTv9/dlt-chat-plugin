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

add_test(NAME tests COMMAND tests)
set_tests_properties(tests PROPERTIES
    ENVIRONMENT "PATH=${CMAKE_PREFIX_PATH}/bin;$ENV{LOCALAPPDATA}/Programs/dlt-viewer/sdk/lib;$ENV{PATH}"
)

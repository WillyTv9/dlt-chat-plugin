set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(quick_action_audit
    main.cpp
    ingestion.cpp
    audit.cpp
    report.cpp
)

target_link_libraries(quick_action_audit
    PRIVATE
        dltchat_app_logic
        qdlt
        ${QT_PREFIX}::Core
        ${QT_PREFIX}::Concurrent
        ${QT_PREFIX}::Gui
        ${QT_PREFIX}::Network
        ${QT_PREFIX}::Xml
)

target_include_directories(quick_action_audit
    PRIVATE "${CMAKE_SOURCE_DIR}/src/app_logic/include"
)

if(WIN32 AND QDLT_RUNTIME)
    add_custom_command(TARGET quick_action_audit POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QDLT_RUNTIME}"
            "$<TARGET_FILE_DIR:quick_action_audit>"
        COMMENT "Copying qdlt.dll next to audit tool"
    )
endif()

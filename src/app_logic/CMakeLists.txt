file(GLOB DLTCHAT_APP_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/dltchat/*.h"
)

add_library(dltchat_app_logic STATIC
    ${DLTCHAT_APP_HEADERS}
    src/analyzer_interface.cpp
    src/llm_analyzer_interface.cpp
    src/bulk_analyzer.cpp
    src/export_engine.cpp
    src/automotive_log_parser.cpp
    src/user_filter_manager.cpp
    src/contextual_extractor.cpp
    src/conversation_manager.cpp
    src/fibex_enricher.cpp
    src/temporal_correlator.cpp
    src/ai_cache_manager.cpp
    src/log_store.cpp
    src/log_index.cpp
    src/category_registry.cpp
    src/hierarchical_summary_store.cpp
    src/model_profile_registry.cpp
    src/context_budget_planner.cpp
    src/enhanced_retriever.cpp
    src/log_ingestion_pipeline.cpp
    src/map_reduce_analyzer.cpp
)

target_sources(dltchat_app_logic PRIVATE
    "${CMAKE_SOURCE_DIR}/src/resources/dltchat_resources.qrc"
)

target_include_directories(dltchat_app_logic
    PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(dltchat_app_logic
    PUBLIC
        ${QT_PREFIX}::Core
        ${QT_PREFIX}::Concurrent
    PRIVATE
        ${QT_PREFIX}::Network
        ${QT_PREFIX}::Xml
)

target_compile_features(dltchat_app_logic PUBLIC cxx_std_17)

set_target_properties(dltchat_app_logic PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

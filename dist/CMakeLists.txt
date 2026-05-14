# Distribution packaging target
#
# Creates a Host-ready bundle containing the plugin binary, config,
# presets, and documentation.

set(DIST_DIR "${CMAKE_BINARY_DIR}/dist/dltchatplugin")

# Determine plugin binary
if(WIN32)
    set(PLUGIN_BINARY "$<TARGET_FILE:dltchatplugin>")
else()
    set(PLUGIN_BINARY "$<TARGET_FILE:dltchatplugin>")
endif()

# Source files to include
set(DIST_CONFIG    "${CMAKE_SOURCE_DIR}/dlt_chat_plugin.ini.example")
set(DIST_PRESETS   "${CMAKE_SOURCE_DIR}/automotive_filters_example.json")
set(DIST_README    "${CMAKE_SOURCE_DIR}/README.md")
set(DIST_LICENSE   "${CMAKE_SOURCE_DIR}/LICENSE")
set(DIST_ARCH      "${CMAKE_SOURCE_DIR}/ARCHITECTURE.md")
set(DIST_PRESET_DIR "${CMAKE_SOURCE_DIR}/src/resources/presets")

add_custom_target(dist
    COMMAND ${CMAKE_COMMAND} -E echo "Packaging plugin to ${DIST_DIR}..."
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy "${PLUGIN_BINARY}" "${DIST_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy "${DIST_CONFIG}"    "${DIST_DIR}/dlt_chat_plugin.ini"
    COMMAND ${CMAKE_COMMAND} -E copy "${DIST_PRESETS}"   "${DIST_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy "${DIST_README}"    "${DIST_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy "${DIST_LICENSE}"   "${DIST_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy "${DIST_ARCH}"      "${DIST_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${DIST_PRESET_DIR}" "${DIST_DIR}/presets"
    COMMENT "Plugin distribution created at ${DIST_DIR}"
    VERBATIM
)

# Add dist to default build if BUILD_DIST is ON
if(DLTCHAT_BUILD_DIST)
    add_dependencies(dist dltchatplugin)
endif()

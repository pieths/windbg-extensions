# This script generates the debug_env_startup_commands.txt file

set(EXTENSIONS
    break_commands
    breakpoints_history
    command_lists
    command_logger
    js_command_wrappers
    mcp_server
)

set(SCRIPTS
    callback_location
    continuation_commands
    type_signatures
)

set(POST_LOAD_COMMANDS
    "!AddBreakCommand !ShowNearbyCommandLists"
    "!ListBreakpointsHistory"
)

# Prepare output content
set(OUTPUT_CONTENT "")

# Add extension load commands
foreach(ext ${EXTENSIONS})
    string(REPLACE "/" "\\\\" BUILD_DIR_ESCAPED "${BINARY_DIR}/build_output")
    string(APPEND OUTPUT_CONTENT ".load \"${BUILD_DIR_ESCAPED}\\\\${ext}.dll\"\n")
endforeach()

# Add script load commands
foreach(script ${SCRIPTS})
    string(REPLACE "/" "\\\\" SCRIPTS_DIR_ESCAPED "${SOURCE_DIR}/scripts")
    string(APPEND OUTPUT_CONTENT ".scriptload \"${SCRIPTS_DIR_ESCAPED}\\\\${script}.js\"\n")
endforeach()

# Add post-load commands
foreach(cmd ${POST_LOAD_COMMANDS})
    string(APPEND OUTPUT_CONTENT "${cmd}\n")
endforeach()

# Write to file
file(WRITE "${SOURCE_DIR}/debug_env_startup_commands.txt" ${OUTPUT_CONTENT})

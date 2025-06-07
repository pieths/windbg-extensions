# This script generates the debug_env_startup_commands.txt file

set(EXTENSIONS
    break_commands
    breakpoints_history
    command_lists
    command_logger
    js_command_wrappers
    mcp_server
    step_through_mojo
)

set(SCRIPTS
    callback_location
    continuation_commands
    type_signatures
)

# Escape the source directory path for use in commands
string(REPLACE "/" "\\\\" SOURCE_DIR_ESCAPED "${SOURCE_DIR}")

set(POST_LOAD_COMMANDS
    # Aliases to make typing specific commands easier.
    "as #bl  !ListBreakpointsHistory"
    "as #bp  !SetBreakpoints"
    "as #bpa !SetAllProcessesBreakpoints"
    "as #sif !StepIntoFunction"
    "as #gcl !GetCallbackLocation"

    "!AddBreakCommand !ShowNearbyCommandLists"
    "!SetBreakpointListsFile \"${SOURCE_DIR_ESCAPED}\\\\breakpoints_history.json\""
    "!SetCommandListsFile \"${SOURCE_DIR_ESCAPED}\\\\command_lists.json\""
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

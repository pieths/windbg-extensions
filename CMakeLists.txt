cmake_minimum_required(VERSION 3.20)
project(WinDbgExtensions)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Windows SDK for dbgeng.lib
find_library(DBGENG_LIB dbgeng)
if(NOT DBGENG_LIB)
    message(FATAL_ERROR "dbgeng.lib not found. Please ensure Windows SDK is installed.")
endif()

# Output directories
# Force single output directory regardless of generator.
# Both debug and release binaries will be put in the build_output directory.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build_output)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build_output)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build_output)

# For multi-configuration generators, override per-config output directories
foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG_TYPE} CONFIG_TYPE_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_BINARY_DIR}/build_output)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_BINARY_DIR}/build_output)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_BINARY_DIR}/build_output)
endforeach()

# Common source files
set(UTILS_SOURCES src/utils.cpp)

# Function to create extension DLL
function(add_windbg_extension name)
    add_library(${name} SHARED ${ARGN} ${UTILS_SOURCES})
    target_link_libraries(${name} PRIVATE ${DBGENG_LIB})
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_NAME ${name}
        LIBRARY_OUTPUT_NAME ${name}
    )
endfunction()

# Native extensions
add_windbg_extension(break_commands src/break_commands.cpp)
add_windbg_extension(breakpoints_history src/breakpoints_history.cpp src/breakpoint_list.cpp)
add_windbg_extension(command_lists src/command_lists.cpp src/command_list.cpp)
add_windbg_extension(command_logger src/command_logger.cpp)
add_windbg_extension(js_command_wrappers src/js_command_wrappers.cpp)
add_windbg_extension(mcp_server src/mcp_server.cpp)
add_windbg_extension(step_through_mojo src/step_through_mojo.cpp)

# Standalone executables
add_executable(mcp_stdio_bridge src/mcp_server_stdio_bridge.cpp)
target_link_libraries(mcp_stdio_bridge PRIVATE ws2_32)

# Tests
enable_testing()
add_subdirectory(tests)

# Custom target to generate startup commands file
add_custom_target(generate_startup_commands ALL
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DBINARY_DIR=${CMAKE_BINARY_DIR}
        -P ${CMAKE_SOURCE_DIR}/cmake/GenerateStartupCommands.cmake
    DEPENDS
        break_commands
        breakpoints_history
        command_lists
        command_logger
        js_command_wrappers
        mcp_server
    COMMENT "Generating debug_env_startup_commands.txt"
)

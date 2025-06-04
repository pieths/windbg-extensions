# Test for break_commands
add_executable(test_break_commands
    test_break_commands.cpp
    ${CMAKE_SOURCE_DIR}/src/utils.cpp
    ${CMAKE_SOURCE_DIR}/src/break_commands.cpp
)
target_link_libraries(test_break_commands PRIVATE ${DBGENG_LIB})
target_compile_definitions(test_break_commands PRIVATE _DEBUG)
target_compile_options(test_break_commands PRIVATE /Zi /Od /MDd)

add_test(NAME break_commands_test COMMAND test_break_commands)

# Test for utils
add_executable(test_utils
    test_utils.cpp
    ${CMAKE_SOURCE_DIR}/src/utils.cpp
)
target_link_libraries(test_utils PRIVATE ${DBGENG_LIB})
target_compile_definitions(test_utils PRIVATE _DEBUG)
target_compile_options(test_utils PRIVATE /Zi /Od /MDd)

add_test(NAME utils_test COMMAND test_utils)

# Test for breakpoint_list
add_executable(test_breakpoint_list
    test_breakpoint_list.cpp
    ${CMAKE_SOURCE_DIR}/src/breakpoint_list.cpp
    ${CMAKE_SOURCE_DIR}/src/utils.cpp
)
target_link_libraries(test_breakpoint_list PRIVATE ${DBGENG_LIB})
target_compile_definitions(test_breakpoint_list PRIVATE _DEBUG)
target_compile_options(test_breakpoint_list PRIVATE /Zi /Od /MDd)

add_test(NAME breakpoint_list_test COMMAND test_breakpoint_list)

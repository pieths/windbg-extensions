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

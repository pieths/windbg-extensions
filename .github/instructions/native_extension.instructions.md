---
applyTo: "**/*.cpp,**/*.h"
---

## Naming Conventions

- Use snake case for local variables, global variables and function parameters
- Suffix private class members with underscore (_)
- Use Pascal case for class names and function names.

## Coding guidelines

- Use `nullptr` instead of `NULL`
- When possible use `std::string` instead of `char*`
- Use `std::vector` instead of C-style arrays
- When it makes sense, try to use the functions defined in `utils.h` and `utils.cpp` to avoid code duplication
- When creating a new extension, keep the overall structure similar to existing extensions
- When creating a new extension, put the relevant source files that need to be built in the `CMakeLists.txt` file and update the `cmake/GenerateStartupCommands.cmake` file to include the new extension in the startup commands generation.
- When creating a new command that is exported from the extension, ensure that it has help text that is printed out to the command window when the user passes "?" as the first argument to the command.
- Put the extern methods at the end of the file wrapped in a `extern "C" {}` block.

## The following files should be used as references for writing new extensions (but not for what those extensions do):

[dbgeng.h](dbgeng.h)
[utils.h](../../src/utils.h)
[utils.cpp](../../src/utils.cpp)
[js_command_wrappers.cpp](../../src/js_command_wrappers.cpp)
[CMakeLists.txt](../../CMakeLists.txt)
[GenerateStartupCommands.cmake](../../cmake/GenerateStartupCommands.cmake)
---
applyTo: '**/*.js'
---

## Naming Conventions

- Use camel case for local variables, global variables and function parameters
- Use pascal case for function names.
- Prefix private function names with double underscore (__)
- Exported (non private) function names should not have a special prefix or suffix.

## Coding guidelines

- When creating a new javascript extension, keep the overall structure similar to existing javascript extensions
- When creating a new javascript extension, put the new javascript file in the "Javascript extensions" section of the `build.ps1` file.
- When creating a new command that is exported from the extension script file (aka. non private function), create a corresponding entry in `js_command_wrappers.cpp`.

## Use the following extra files for context

[callback_location.js](../../scripts/callback_location.js)
[continuation_commands.js](../../scripts/continuation_commands.js)
[type_signatures.js](../../scripts/type_signatures.js)
[js_command_wrappers.cpp](../../src/js_command_wrappers.cpp)
[build.ps1](../../build.ps1)

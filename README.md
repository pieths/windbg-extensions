# WinDbg Extensions for Chromium Developers

**NOTE**: This project is still in active development and some features
may be experimental or subject to change.

## Overview

This project serves as a development environment for creating basic
WinDbg extensions, providing templates, build infrastructure, and examples to
help developers quickly build their own debugging tools.

It includes examples of both C++ native extensions and JavaScript
extensions for WinDbg, designed to streamline common debugging tasks and provide
automation capabilities. The sample extensions are geared towards debugging
Chromium but might also be useful for debugging other applications.

## Sample Extensions Overview

### **Breakpoint Management**
- **Breakpoint History**: Persistent storage and management of breakpoint collections
  with tagging and search capabilities
- **Smart Breakpoint Setting**: Support for source file:line breakpoints with automatic
  path escaping
- **Bulk Operations**: Set breakpoints across multiple processes simultaneously

### **Command Automation**
- **Break Event Commands**: Automatically execute commands when the debugger breaks
  (step over, step into, etc.)
- **Command Lists**: Record, replay, and manage sequences of debugging commands with
  source location awareness

### **General Utility Commands**
- **Smart Step Into**: Advanced stepping with function filtering and pattern matching
- **Callback Location Analysis**: Analyze Chrome callback objects to determine actual
  callback locations
- **Continuation Commands**: Enhanced navigation with automatic breakpoint setting

### **MCP Server**
- Model Context Protocol (MCP) server for AI-assisted debugging (EXPERIMENTAL)
- Integration with VS Code and other MCP-compatible tools

See [Commands Reference](docs/commands.md) for more details about the available extension commands.

## Copilot Integration

This project is designed to work seamlessly with GitHub Copilot in Visual Studio
Code. The codebase includes specialized instruction files in
`.github\instructions\` that provide Copilot with context-aware guidance for
maintaining consistent coding patterns and conventions across both C++ and
JavaScript extensions. The project structure, naming conventions, and build
system are specifically organized to make AI-assisted development intuitive,
allowing Copilot to suggest appropriate code completions, generate new
extensions following established patterns, and maintain consistency with the
existing codebase architecture.

## Requirements

- **Visual Studio 2019/2022** with C++ development tools (command line only version is okay)
- **Windows SDK** (10.0.26100.0 or compatible)
- **WinDbg** (Debugging Tools for Windows)
- **PowerShell** (for build scripts)

## Quick Start

### Building the Extensions

Run the build script from the project root:

```powershell
.\build.ps1
```

This will compile all C++ extensions and place them in the `build_output`
directory. It also auto-generates a WinDbg startup script which can be
used to launch WinDbg with all the scripts loaded.

**Visual Studio Code Integration**: If you're using VS Code, you can also build
*using the pre-configured tasks. Press `Ctrl+Shift+P`, type "Tasks: Run Task",
*and select "Build WinDbg Extensions" or use `Ctrl+Shift+B` for the default
*build task.

**Note**: The build script is designed for easy extension development - simply
*add new C++ extensions to the `$extensions` array or JavaScript files to the
`$scripts` array at the top of `build.ps1` to automatically include them in the
build process. These details are included in the Copilot instructions.md files
and Copilot should automatically do this for you when it is asked to create
a new extension.

### Loading Extensions in WinDbg

1. **Automatic Loading**: Use the auto-generated
   [debug_env_startup_commands.txt](debug_env_startup_commands.txt) file:
   ```
   C:\path\to\WinDbgX.exe -c "$$<D:\windbg\extensions\debug_env_startup_commands.txt"
   ```
   You can add these command line parameters to a WinDbg shortcut to automatically
   load the extensions when WinDbg starts.

2. **Manual Loading**: Load individual extensions in WinDbg:
   ```
   .load D:\windbg\extensions\build_output\break_commands.dll
   .load D:\windbg\extensions\build_output\breakpoints_history.dll
   .scriptload D:\windbg\extensions\scripts\continuation_commands.js
   ```

### Basic Usage Examples

```windbg
# Set up automatic stack traces on breaks
!AddBreakCommand k

# Record a command sequence
!StartCommandListRecording MyCommands "Debug session commands"
# ... execute your debugging commands ...
!StopCommandListRecording

# Set breakpoints from history
!ListBreakpointsHistory
!SetBreakpoints 0  # Use first breakpoint list

# Smart step into upcoming function call containing text pattern "CreateDevice"
!StepIntoFunction CreateDevice
```

## Project Structure

```
├── src/                         # C++ native extensions source
│   ├── break_commands.cpp       # Break event command automation
│   ├── breakpoints_history.cpp  # Persistent breakpoint management
│   ├── command_lists.cpp        # Command sequence recording/replay
│   ├── command_logger.cpp       # Command logging utilities
│   └── utils.cpp                # Shared utility functions
│   └── ...
├── scripts/                     # JavaScript extensions
│   ├── continuation_commands.js # Enhanced stepping and navigation
│   ├── callback_location.js     # Chrome callback analysis
│   └── type_signatures.js       # Custom object visualizers
├── docs/                        # Documentation
└── build_output/                # Compiled extensions (generated)
```

## Clangd Language Server

This project uses `clangd` as the language server for formatting and code
completion in Visual Studio Code. The `.clangd` and `.clang-format` (clang
format is built-in to clangd) configuration files in the project root ensures
consistent formatting and provides enhanced IntelliSense support for the C++
extensions.

### Installing Clangd

Clangd can be download from [clangd.llvm.org](https://clangd.llvm.org/installation).

Alternatively, if you have a Chromium repository, you can use its clangd.  It
will be located in the
`[CHROMIUM_REPO_ROOT]/src/third_party/llvm-build/Release+Asserts/bin/`
directory.  If the executable is not there then it can be retrieved by adding
`'checkout_clangd': True` to the `custom_vars` in the
`[CHROMIUM_REPO_ROOT]/.gclient` file and then running `gclient sync`.
  ```
  solutions = [
    {
      ...
      "custom_vars": {'checkout_clangd': True},
    },
  ]
  ```


### Install VS Code Extension

Install the official [clangd extension](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd) from the VS Code marketplace:
- Open VS Code
- Go to Extensions (Ctrl+Shift+X)
- Search for "clangd" by LLVM
- Click Install

### Update VS Code Settings

Update `.vscode/settings.json` to point to the clangd executable:
```json
"clangd.path": "d:/cs/src/third_party/llvm-build/Release+Asserts/bin/clangd.exe"
```

If you have the Microsoft C/C++ extension installed, disable it for this workspace to avoid conflicts with clangd. The `settings.json` in this project
already has this disabled so nothing should need to be done here.

## Documentation

- **[Commands Reference](docs/commands.md)** - Complete list of available extension
  commands
- **[Development Guide](docs/windbg_extensions_guide.md)** - Creating custom extensions
- **[MCP Integration](docs/mcp_server_setup.md)** - AI-assisted
  debugging setup

## 3rd Party Licenses

The following 3rd party software is included.
Please see their respective directories and/or files for their license terms.

* json (nlohmann)

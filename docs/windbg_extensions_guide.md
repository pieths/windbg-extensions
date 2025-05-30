# WinDbg Extensions Development Guide

This guide provides an overview of creating WinDbg extensions, covering both C++ native
extensions and JavaScript extensions. The examples in this project demonstrate practical
implementation patterns and techniques that can serve as a learning foundation for
developing your own extensions.

## Table of Contents

- [Overview](#overview)
- [C++ Native Extensions](#c-native-extensions)
- [JavaScript Extensions](#javascript-extensions)

## Overview

WinDbg extensions allow you to add custom commands and functionality to the debugger.
This project contains two types of extensions:

1. **C++ Native Extensions** - Compiled DLLs that export functions callable from WinDbg
2. **JavaScript Extensions** - Script files that provide debugger automation and
   custom object visualization

## C++ Native Extensions

### Basic Structure

A C++ extension is a DLL that exports specific functions. At minimum, you need:

```cpp
// Required initialization and cleanup functions
extern "C" {
    __declspec(dllexport) HRESULT CALLBACK
    DebugExtensionInitialize(PULONG version, PULONG flags);

    __declspec(dllexport) HRESULT CALLBACK
    DebugExtensionUninitialize(void);

    // Your custom commands
    __declspec(dllexport) HRESULT CALLBACK
    YourCommand(IDebugClient* client, const char* args);
}
```

Note, this is just one way of creating DLL exports. This techique was chosen for
the extensions in this project to keep everything as simple and self-contained
as possible.

### Example: Simple Command Extension

Here's a minimal example based on the `break_commands.cpp` structure:

```cpp
#include <windows.h>
#include <dbgeng.h>
#include <string>
#include "utils.h"

utils::DebugInterfaces g_debug;

HRESULT CALLBACK DebugExtensionInitializeInternal(PULONG version, PULONG flags) {
    *version = DEBUG_EXTENSION_VERSION(1, 0);
    *flags = 0;
    return utils::InitializeDebugInterfaces(&g_debug);
}

HRESULT CALLBACK DebugExtensionUninitializeInternal() {
    return utils::UninitializeDebugInterfaces(&g_debug);
}

HRESULT CALLBACK HelloWorldInternal(IDebugClient* client, const char* args) {
    if (args && args[0] == '?' && args[1] == '\0') {
        DOUT("HelloWorld - A simple example command\n\n"
             "Usage: !HelloWorld [message]\n\n"
             "Examples:\n"
             "  !HelloWorld           - Print default message\n"
             "  !HelloWorld 'Hello!'  - Print custom message\n");
        return S_OK;
    }

    std::string message = args ? args : "Hello from WinDbg extension!";
    DOUT("%s\n", message.c_str());
    return S_OK;
}

extern "C" {
    __declspec(dllexport) HRESULT CALLBACK
    DebugExtensionInitialize(PULONG version, PULONG flags) {
        return DebugExtensionInitializeInternal(version, flags);
    }

    __declspec(dllexport) HRESULT CALLBACK
    DebugExtensionUninitialize(void) {
        return DebugExtensionUninitializeInternal();
    }

    __declspec(dllexport) HRESULT CALLBACK
    HelloWorld(IDebugClient* client, const char* args) {
        return HelloWorldInternal(client, args);
    }
}
```

### Key Components

#### 1. Debug Interfaces

This project uses the `utils::DebugInterfaces` structure to access WinDbg's COM
interfaces. This structure provides convenient access to the most commonly used
debugger interfaces.

```cpp
utils::DebugInterfaces g_debug;
// Provides access to:
// - g_debug.client   (IDebugClient*)
// - g_debug.control  (IDebugControl*)
// - g_debug.symbols  (IDebugSymbols*)
```

There are many more COM interfaces available.
See [here](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/dbgeng/)
for more details.

These interfaces serve as the communication bridge between your extension and the
debugger engine, providing the primary API for all debugger interactions.

#### 2. Output Macros

The following macros can be used to output text to the command window.
They are defined in `utils.h` and are designed to work with the `g_debug`
global variable shown above. These macros provide convenient wrappers around
`IDebugControl::Output` and help keep the code less verbose.

```cpp
DOUT("Normal output: %s\n", message.c_str());     // DEBUG_OUTPUT_NORMAL
DERROR("Error: %s\n", error_message.c_str());     // DEBUG_OUTPUT_ERROR
```

#### 3. Argument Parsing

By default a C++ native extension receives a single string which contains
all the arguments passed into the command by the user. While this allows
for the greatest amount of flexibility in how args can be represented, it
requires more work on the part of the extension to properly parse the args.

The `utils::ParseCommandLine()` method in this project provides an
argument parser which uses white space to delimit arguments and
single quotes (') to delimit args that contain white space.
See the `ParseCommandLine` comments in `utils.h` for why single
quotes were chosen over double quotes.

```cpp
std::vector<std::string> parsed_args = utils::ParseCommandLine(args);
// Supports single-quoted strings: !cmd 'arg with spaces' arg2
```

#### 4. Help System

Though not technically required, it can be beneficial for users of the command
to implement some sort of "help" output. The extensions in this project use
the `?` argument to display help text.

```cpp
if (args && args[0] == '?' && args[1] == '\0') {
    DOUT("CommandName - Brief description\n\n"
         "Usage: !CommandName [options]\n\n"
         "Examples:\n"
         "  !CommandName option1  - Description\n");
    return S_OK;
}
```

### Advanced Features

#### Event Callbacks

For handling debugger events (like breaks), an extension can implement
`IDebugEventCallbacks` (or the WinDbg SDK provided derived class
`DebugBaseEventCallbacks`).
This interface allows your extension to respond to various debugger events
such as breakpoints, exceptions, process creation/termination, and
execution state changes.

```cpp
class MyEventHandler : public DebugBaseEventCallbacks {
public:
    STDMETHODIMP ChangeEngineState(ULONG flags, ULONG64 argument) {
        if (flags & DEBUG_CES_EXECUTION_STATUS) {
            if (argument == DEBUG_STATUS_BREAK) {
                // Handle break event
            }
        }
        return S_OK;
    }
};
```

See `break_commands.cpp` for a more complete implementation.

### Reference Files

- **Basic Extension**: `break_commands.cpp` - Simple command with event handling
- **Complex Extension**: `breakpoints_history.cpp` - File I/O, JSON, argument parsing
- **Utility Functions**: `utils.h` and `utils.cpp` - Helper functions and interfaces

## JavaScript Extensions

### Basic Structure

JavaScript extensions are script files loaded with `.scriptload` or `.scriptrun`.
They can provide:

- Custom functions accessible via `dx` commands
- Object visualizers for custom types
- Debugger automation scripts

See [here](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/javascript-debugger-scripting) for more details.

### Example: Simple JavaScript Extension

```javascript
"use strict";

function MyFunction(param1, param2) {
    let ctl = host.namespace.Debugger.Utility.Control;

    host.diagnostics.debugLog("MyFunction called with: " + param1 + "\n");

    // Execute WinDbg commands
    ctl.ExecuteCommand("k");
}

function initializeScript() {
    return [
        new host.functionAlias(MyFunction, "MyFunction"),
    ];
}
```

When a `functionAlias` is created, the JavaScript command can then be called using the
standard WinDbg extension syntax:

```
!MyFunction 1, 2
```

However, note that arguments must be separated by commas and follow JavaScript syntax
rules. String arguments that don't reference debugger symbols must be quoted with
double quotes. This differs from the more flexible argument parsing available with
native C++ extensions.

Alternatively, you can call the function directly using the `dx` command.
If the script file is called `my_script.js` and was loaded into WinDbg using
`.scriptload` then the function could also be accessed like this:

```
dx Debugger.State.Scripts.my_script.Contents.MyFunction("test", 123)
```

### Debugger Instance

The `dx Debugger` command provides access to the debugger's object model and can be used
to explore available properties, state, and functionality. This is particularly useful
for understanding what's available in JavaScript extensions.

**Some Debugger Object Properties:**

```javascript
// Access current execution state
let currentFrame = host.namespace.Debugger.State.DebuggerVariables.curframe;
let currentThread = host.namespace.Debugger.State.DebuggerVariables.curthread;
let currentProcess = host.namespace.Debugger.State.DebuggerVariables.curprocess;

// Access loaded scripts
let scripts = host.namespace.Debugger.State.Scripts;
let myScript = scripts.my_script_name.Contents;

// Access debugger utilities
let ctl = host.namespace.Debugger.Utility.Control;
let memory = host.namespace.Debugger.Utility.Memory;
```

**Useful Exploration Commands:**

Here are some example commands that can be run in the command window
to explore some of the available properties and functionality.

```
dx Debugger                                  # View top-level debugger object
dx Debugger.State                            # View current debugger state
dx Debugger.State.DebuggerVariables          # View built-in debugger variables
dx Debugger.State.Scripts                    # View loaded JavaScript extensions
dx Debugger.Utility                          # View utility functions
dx Debugger.Sessions                         # View debugging sessions
```

**Practical Examples:**

```javascript
// Get current instruction pointer
let ip = host.namespace.Debugger.State.DebuggerVariables.curframe.Attributes.InstructionOffset;

// Get current source location
let sourceInfo = host.namespace.Debugger.State.DebuggerVariables.curframe.Attributes.SourceInformation;
let fileName = sourceInfo.SourceFile;
let lineNumber = sourceInfo.SourceLine;

// Execute commands and capture output
let ctl = host.namespace.Debugger.Utility.Control;
let output = ctl.ExecuteCommand("k");
for (let line of output) {
    host.diagnostics.debugLog(line.toString() + "\n");
}

// Access memory
let memoryValue = host.memory.readMemoryValues(address, 1, 8)[0];
```

**Script State Access:**

When scripts are loaded with `.scriptload`, they become accessible through the
Debugger object:

```javascript
// If you loaded "my_extension.js"
dx Debugger.State.Scripts.my_extension.Contents.MyFunction("arg1", "arg2")

// Access script variables and state
dx Debugger.State.Scripts.my_extension.Contents.myGlobalVariable
```

This object model provides the foundation for all JavaScript extension functionality
and serves as the primary interface between your scripts and the debugger engine.

> Note, global variables and methods in the script which are prefixed with
> double underscore `__` will not be visible outside of the script.

### Accessing Native Objects

When an input parameter to a JavaScript method is a native object instance
in the target application, the member variables are available as properties
on the object. This allows you to directly access and manipulate C++ object
data from JavaScript.

```javascript
function AnalyzeCallback(callbackObject) {
    // Access member variables directly as properties
    let bindState = callbackObject.holder_.bind_state_;
    let address = bindState.Ptr.address;

    host.diagnostics.debugLog("BindState address: " + address.toString(16) + "\n");

    // Navigate through nested objects
    let taskRunner = callbackObject.task_runner_;
    if (taskRunner) {
        host.diagnostics.debugLog("Task runner type: " + taskRunner.targetType + "\n");
    }
}
```

This direct object access capability makes JavaScript extensions particularly powerful
for analyzing complex C++ data structures, as demonstrated in the `callback_location.js`
and `continuation_commands.js` examples in this project.

### Object Visualizers

Create custom string representations for C++ types. These strings will be
displayed whenever the variable is shown in the locals window or with the `dx`
command. This can be particularly useful for surfacing important object properties
that aren't immediately visible, making debugging faster by eliminating the need
to manually drill down into object members.

```javascript
class MyClass_Visualizer {
    toString() {
        return "MyClass: " + this.member1.toString() +
               ", " + this.member2.toString();
    }
}

function initializeScript() {
    return [
        new host.typeSignatureRegistration(MyClass_Visualizer, "MyNamespace::MyClass"),
    ];
}
```

See `type_signatures.js` for complete examples.

### Debugger Automation

Access debugger state and control execution:

```javascript
function StepToFunction(functionName) {
    let ctl = host.namespace.Debugger.Utility.Control;

    // Get current frame information
    let currentFrame = host.namespace.Debugger.State.DebuggerVariables.curframe;
    let currentIp = currentFrame.Attributes.InstructionOffset;

    // Find function calls in current function
    let ufOutput = ctl.ExecuteCommand("uf /c @rip");

    // Parse output and find matching function
    for (let line of ufOutput) {
        if (line.toString().includes(functionName)) {
            // Set breakpoint and continue
            ctl.ExecuteCommand("bp " + address);
            ctl.ExecuteCommand("g");
            break;
        }
    }
}
```

See `continuation_commands.js` for advanced examples.

### JavaScript Command Wrappers

For better argument handling, wrap JavaScript functions with C++ commands.
This approach provides several important benefits that improve the user experience
and maintain consistency across all commands in the project.

**Why JavaScript Command Wrappers Are Useful:**

1. **Consistent Argument Syntax**: JavaScript functions exposed via `host.functionAlias`
   require comma-separated arguments and strict JavaScript syntax rules. This creates
   inconsistency with native C++ extensions and standard WinDbg commands.

2. **String Argument Handling**: Direct JavaScript function calls require double quotes
   around string arguments that don't reference debugger symbols, which is unintuitive
   for users familiar with WinDbg command syntax.

3. **Flexible Argument Parsing**: C++ wrappers can use the project's `ParseCommandLine()`
   utility to handle single-quoted strings with spaces, providing more flexible and
   user-friendly argument parsing.

```cpp
std::string BuildMyCommandArguments(const char* args) {
    std::string jsCommand = "dx @$scriptContents.MyFunction(";
    std::vector<std::string> parsed_args = utils::ParseCommandLine(args);

    // Parse and format arguments for JavaScript
    if (!parsed_args.empty()) {
        jsCommand += "\"" + parsed_args[0] + "\"";
    }

    jsCommand += ")";
    return jsCommand;
}
```
**Comparison Example:**

```javascript
// Direct JavaScript call (less user-friendly)
!StepIntoFunction "InitializeComponent", 42

// With C++ wrapper (more user-friendly and less typing)
!StepIntoFunction 'Initialize Component' 42
!StepIntoFunction InitializeComponent 42
```

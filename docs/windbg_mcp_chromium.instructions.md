# MCP Server for WinDbg - AI Client Instructions

## Overview

This document provides instructions for AI clients to interact with the MCP
(Model Context Protocol) server that bridges communication with WinDbg (Windows
Debugger). The server uses standard MCP protocol over TCP or stdio with
JSON-RPC 2.0 and newline-delimited messages.

## Connection Details

- **Protocol**: TCP
- **Message Format**: JSON-RPC 2.0 (MCP Protocol)
- **Message Delimiter**: Newline (`\n`)
- **Default Port**: Automatic (0) - server will choose an available port
- **Connection String**: `tcp://localhost:<port>`

## Starting the Server

Before connecting, ensure the MCP server is running in WinDbg:

```
!StartMCPServer        # Start on automatic port
!StartMCPServer 8080   # Start on specific port
!MCPServerStatus       # Check server status
!StopMCPServer         # Stop the server
```

## MCP Protocol

The server implements the standard MCP protocol with the following methods:

### 1. initialize

Initializes the MCP connection and returns server capabilities.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "initialize",
  "params": {
    "protocolVersion": "0.1.0",
    "capabilities": {},
    "clientInfo": {
      "name": "AI Client",
      "version": "1.0.0"
    }
  },
  "id": 1
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "protocolVersion": "0.1.0",
    "capabilities": {
      "tools": {
        "listChanged": true
      },
      "prompts": {}
    },
    "serverInfo": {
      "name": "windbg-mcp-server",
      "version": "1.0.0"
    }
  }
}
```

### 2. initialized

Notification sent after successful initialization (no response expected).

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "initialized"
}
```

### 3. tools/list

Lists all available tools that can be called.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "tools/list",
  "params": {},
  "id": 2
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 2,
  "result": {
    "tools": [
      {
        "name": "executeCommand",
        "description": "Execute a WinDbg command asynchronously",
        "inputSchema": {
          "type": "object",
          "properties": {
            "command": {
              "type": "string",
              "description": "The WinDbg command to execute"
            }
          },
          "required": ["command"]
        }
      },
      {
        "name": "getOperationStatus",
        "description": "Check the status of an asynchronous operation",
        "inputSchema": {
          "type": "object",
          "properties": {
            "operation_id": {
              "type": "string",
              "description": "The operation ID to check"
            }
          },
          "required": ["operation_id"]
        }
      },
      {
        "name": "getDebuggerState",
        "description": "Get the current debugger state",
        "inputSchema": {
          "type": "object",
          "properties": {}
        }
      }
    ]
  }
}
```

### 4. tools/call

Calls a specific tool with the provided arguments.

## Available Tools

### executeCommand

Executes a WinDbg command asynchronously. Returns immediately with an operation
ID that must be polled for results.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "executeCommand",
    "arguments": {
      "command": "k"
    }
  },
  "id": 3
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 3,
  "result": {
    "content": [
      {
        "type": "text",
        "text": "Command queued for execution. Operation ID: op_42_17321895476290000"
      }
    ],
    "meta": {
      "operation_id": "op_42_17321895476290000",
      "status": "pending",
      "type": "async_operation",
      "metadata": {
        "async": true,
        "poll_method": "getOperationStatus",
        "poll_interval_ms": 100
      }
    }
  }
}
```

### getOperationStatus

Polls for the status of an asynchronous operation.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "getOperationStatus",
    "arguments": {
      "operation_id": "op_42_17321895476290000"
    }
  },
  "id": 4
}
```

**Response (Completed):**
```json
{
  "jsonrpc": "2.0",
  "id": 4,
  "result": {
    "content": [
      {
        "type": "text",
        "text": "Operation completed:\nChild-SP          RetAddr           Call Site\n00000000`0012f8a8 00007ff8`12345678 ntdll!NtWaitForSingleObject+0x14\n..."
      }
    ]
  }
}
```

**Response (Pending):**
```json
{
  "jsonrpc": "2.0",
  "id": 4,
  "result": {
    "content": [
      {
        "type": "text",
        "text": "Operation is still pending..."
      }
    ]
  }
}
```

**Response (Not Found):**
```json
{
  "jsonrpc": "2.0",
  "id": 4,
  "result": {
    "content": [
      {
        "type": "text",
        "text": "Operation not found"
      }
    ]
  }
}
```

**Response (Error):**
```json
{
  "jsonrpc": "2.0",
  "id": 4,
  "result": {
    "content": [
      {
        "type": "text",
        "text": "Error: No operation_id specified"
      }
    ]
  }
}
```

### getDebuggerState

Gets the current state of the debugger synchronously.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "getDebuggerState",
    "arguments": {}
  },
  "id": 5
}
```

**Response (Break State):**
```json
{
  "jsonrpc": "2.0",
  "id": 5,
  "result": {
    "content": [
      {
        "type": "text",
        "text": "Debugger state: break"
      }
    ]
  }
}
```

**Response (No Debuggee):**
```json
{
  "jsonrpc": "2.0",
  "id": 5,
  "result": {
    "content": [
      {
        "type": "text",
        "text": "Debugger state: no_debuggee"
      }
    ]
  }
}
```

**Response (Running):**
```json
{
  "jsonrpc": "2.0",
  "id": 5,
  "result": {
    "content": [
      {
        "type": "text",
        "text": "Debugger state: running"
      }
    ]
  }
}
```

**Response (Stepping):**
```json
{
  "jsonrpc": "2.0",
  "id": 5,
  "result": {
    "content": [
      {
        "type": "text",
        "text": "Debugger state: stepping"
      }
    ]
  }
}
```

**Response (Error):**
```json
{
  "jsonrpc": "2.0",
  "id": 5,
  "result": {
    "content": [
      {
        "type": "text",
        "text": "Error: Failed to get execution status"
      }
    ]
  }
}
```

## Standard Error Codes

- `-32700`: Parse error - Invalid JSON
- `-32601`: Method not found
- `-32603`: Internal error

## __REQUIRED__ `executeCommand` Workflow

### Pseudo Code for AI Clients

Follow this exact workflow when executing WinDbg commands with `executeCommand`:

```pseudocode
function executeWinDbgCommand(command_string):
    // Step 1: Always check debugger state first
    debugger_state = call_tool("getDebuggerState", {})

    if debugger_state.content[0].text does not contain "break":
        if debugger_state.content[0].text contains "no_debuggee":
            return error("No debuggee attached. Cannot execute commands.")
        elif debugger_state.content[0].text contains "running":
            return error("Debugger is running. Trying running the command again later when the debugger is in the break state.")
        elif debugger_state.content[0].text contains "stepping":
            return error("Debugger is stepping. Wait for step to complete. Try again later.")
        else:
            return error("Debugger not in break state. Current state: " + debugger_state)

    // Step 2: Execute the command asynchronously
    execute_response = call_tool("executeCommand", {"command": command_string})

    if execute_response.isError == true:
        return error(execute_response.content[0].text)

    // Step 3: Extract operation ID from response
    operation_id = execute_response.meta.operation_id
    poll_interval = execute_response.meta.metadata.poll_interval_ms  // Usually 100ms

    // Step 4: Poll for results
    max_polls = 3000  // 5 minutes timeout (3000 * 100ms = 300 seconds)
    poll_count = 0

    while poll_count < max_polls:
        sleep(poll_interval)  // Wait recommended interval
        poll_count += 1

        status_response = call_tool("getOperationStatus", {"operation_id": operation_id})

        if status_response.content[0].text starts with "Operation completed:":
            // Success - extract the command output
            command_output = status_response.content[0].text.replace("Operation completed:\n", "")
            return success(command_output)

        elif status_response.content[0].text == "Operation is still pending...":
            // Continue polling
            continue

        elif status_response.content[0].text == "Operation not found":
            return error("Operation timed out or was cleaned up")

        elif status_response.content[0].text starts with "Error:":
            return error(status_response.content[0].text)

        else:
            // Unknown status - abort and retry
            return error("Unknown operation status: " + status_response.content[0].text)

    // Timeout reached
    return error("Command execution timed out after 5 minutes")

// Helper function for tool calls
function call_tool(tool_name, arguments):
    request = {
        "jsonrpc": "2.0",
        "method": "tools/call",
        "params": {
            "name": tool_name,
            "arguments": arguments
        },
        "id": generate_unique_id()
    }

    response = send_request(request)
    return response.result

// Example usage:
result = executeWinDbgCommand("k")  // Get stack trace
if result.success:
    print("Stack trace:", result.data)
else:
    print("Error:", result.error_message)
```

### Critical Requirements

1. **ALWAYS** check debugger state with `getDebuggerState` before running `executeCommand`
2. **NEVER** execute commands when debugger state is not "break"
3. **ALWAYS** poll `getOperationStatus` until you get "completed", "not_found", or an error
4. **NEVER** assume a command completed without polling for status
5. **ALWAYS** respect the `poll_interval_ms` to avoid overwhelming the server
6. **ALWAYS** handle timeouts gracefully (5 minute maximum)

### Error Recovery

If any step fails:
- For debugger state errors: Wait or break execution as needed
- For operation errors: Retry the entire workflow from Step 1
- For timeout errors: Consider the command failed and start fresh
- For "not_found" errors: The operation was cleaned up, start fresh

## WinDbg Commands

When using the `executeCommand` tool, you can execute any of the following WinDbg commands.

**IMPORTANT**: Only execute commands from the list below. If you need to execute
a command that is not listed here, first display the command you want to run
and explain why you need it, then ask the user for permission before
proceeding.

The command output will contain the raw output from WinDbg, which may include
stack traces, register values, etc. These should be parsed by the client as
needed.

### Set Breakpoints

When starting a new debugging session, initial breakpoints need to **ALWAYS**
be set with something like the following so that the breakpoints get set for all
child processes.

```
.childdbg 1; sxn ibp; sxn epr; sxe -c "bp chrome!media::CreateMediaFoundationCdm; gc" ld:chrome.dll; g
```

Multiple breakpoints can be set in the same command by separating them with
semicolons (`;`). Here is an example of setting multiple breakpoints:

```
.childdbg 1; sxn ibp; sxn epr; sxe -c "bp chrome!media::CreateMediaFoundationCdm; bp chrome!media::CreateMediaFoundationCdm::Initialize; gc" ld:chrome.dll; g
```

After the initial breakpoints are set and a new breakpoint is hit, breakpoints
can be set in the child processes using the standard `bp` command as described above.

#### Breakpoint Format 1

```
bp chrome!media::CreateMediaFoundationCdm
```

This sets a breakpoint at the `CreateMediaFoundationCdm` function in the `chrome.dll` module.
The fully qualified function name and module is required for the breakpoint to be set correctly.
If the symbol where the breakpoint is being set is inside of an anonymous namespace,
then use the SourceFile:Line format to set the breakpoint.

When specifying the module name, do not use the full path to the module, just the module name
(e.g., `chrome!` for the `chrome.dll` module).

#### Breakpoint Format 2 (SourceFile:Line)

```
bp `chrome!D:\\cs\\src\\content\\browser\\media\\key_system_support_win.cc:79`
```

This sets a breakpoint at line 79 in the `key_system_support_win.cc` source file.
This format is useful when:
- The function is inside of an anonymous namespace
- The function name is not unique enough to set a breakpoint directly
- When setting a breakpoint that is inside of a function (not at the start of a function).

Note, the double backslashes (`\\`) are required for the file path separators in this command.

#### Breakpoint Format 3

```
bp 0x7ffec3b1d9f0
bp 00007ffe`c3b1d86e
```

This sets a breakpoint at the specified memory address. This is useful for
setting breakpoints at specific addresses in the process memory, such as when
you know the address of a function or variable.
Note, line numbers do not work with this format, only addresses.

### Get Symbol Type

```
dt cdm_capability_cb
```

This command retrieves the type information for the local variable
`cdm_capability_cb` symbol.
If the symbol is a global symbol (not a function
local symbol) then use the fully qualified name like `chrome!cdm_capability_cb`
to retrieve the type information.

### Get Stack trace

```
k
```

This command retrieves the current stack trace. It shows the call stack of the
current thread, including function names, addresses, and parameters.

### Examine Module Symbols

```
x chrome!mojo::Message::Message
```

Example response (shortened for brevity):
```
00007ffe`c7d0c970 chrome!mojo::Message::Message (void)
00007ffe`c7d0c9c0 chrome!mojo::Message::Message (class mojo::Message *)
00007ffe`c7d0cb40 chrome!mojo::Message::Message (class std::__Cr::uniq...
00007ffe`c7d0cf20 chrome!mojo::Message::Message (unsigned int, unsigne...
00007ffe`c7d0d730 chrome!mojo::Message::Message (unsigned int, unsigne...
```

This command lists all symbols matching the specified pattern in the `chrome` module.
This command should always be used with a fully qualified symbol name and module name.

### List modules

```
lm
```

This command lists all loaded modules in the current process. It shows the module names, base addresses, and sizes.

### Get Module Details

```
!lmi chrome.dll
```
This command retrieves detailed information about the `chrome.dll` module,
including its base address and full image name including the path.

### List registers

```
r
```

This command lists the current values of all CPU registers.

### List local variables

```
dv
```

This command lists all local variables in the current stack frame.
If the value is displayable, it will also show the value of the variable.

### Get Single Local Variable Value

```
dx <expression>
```

Where `<expression>` is a valid expression that evaluates to a local variable.

When using this command command to retrieve a string value, the actual
string value will be the first quoted string in the output. For example,
if the output is:
```
key_system : "com.microsoft.playready.recommendation" [Type: std::__Cr:...
```
then the string value is `"com.microsoft.playready.recommendation"`.

### Step Over

```
p
```

This command steps over the next instruction in the current thread. It executes
the current line and moves to the next line without stepping into function
calls.

When a breakpoint is hit and the current instruction is at the start of a
function, it is usually required to execute `p` once so that the local
variables are available for inspection.

### Step Into

```
t
```

This command steps into the next instruction in the current thread. If the
current instruction is a function call, it will step into the function and show
the first instruction of the function.

### Continue Execution

```
g
```

This command continues the execution of the process until the next breakpoint is
hit or the process exits.

### Get Current Source file

```
dx Debugger.State.DebuggerVariables.curframe.Attributes.SourceInformation.SourceFile
```

This command retrieves the current source file being debugged. It uses the
`Debugger.State.DebuggerVariables` object to access the current frame's source
file information. The output will be the full path to the source file currently
being executed in the debugger.

### Get Current Source Line Number

```
dx Debugger.State.DebuggerVariables.curframe.Attributes.SourceInformation.SourceLine,d
```

This command retrieves the current source line number being executed in the debugger.

### Get Current Source Line

```
lsa .
```

This command lists the current source line along with a few extra lines of
context before and after the current line. The current line will be prefixed
with a `>` character.

### Get Current Module Name and Address

```
dx -r1 Debugger.State.DebuggerVariables.curframe.Attributes.SourceInformation.Module
```

This command retrieves the current module name being executed in the debugger.

### Get Current Function Name

```
kc 1
```

This command retrieves the current function name being executed in the debugger.

### Access Class Instance Member variables

```
dx this->is_os_cdm_
```

This command retrieves the value of the `is_os_cdm_` member variable of the
current class instance.

```
dx this->com_initializer_.hr_
```

This command retrieves the value of the `hr_` member variable of the `com_initializer_`
member variable of the current class instance.

### Get Callback Details

```
!GetCallbackLocation callback
```

Example response:
```
bp 0x7ffebf6be680
bp /1 0x7ffebf6be680

The following symbol was found for the given functor:
Note, this symbol might not be correct.
Use the hex address provided above for the actual callback location when unsure.

chrome!media::mojom::MediaFoundationService_IsKeySystemSupported_ProxyToResponder::Run
D:\cs\src\out\release_x64\gen\media\mojo\mojom\media_foundation_service.mojom.cc:549
D:\\cs\\src\\out\\release_x64\\gen\\media\\mojo\\mojom\\media_foundation_service.mojom.cc:549
`D:\\cs\\src\\out\\release_x64\\gen\\media\\mojo\\mojom\\media_foundation_service.mojom.cc:549`

Debugger.State.Scripts.callback_location.Contents.GetCallbackLocation(callback)
```

This command retrieves the location of a callback function in the Chromium codebase.
The result includes the address of the callback function, the symbol name, and the source file
and line number where the callback is defined.

To set a breakpoint at the callback location, use the following format of the command:

```
!GetCallbackLocation callback bp1
!GetCallbackLocation callback bp1g
```

Where the `bp1` command sets a breakpoint at the callback location and the
`bp1g` command sets a breakpoint and continues execution.

### Run To Line In Current function

```
!g 23
```

This command runs the debugger to the specified line number in the current function.
It is required that the line specified is in the current function being executed.

### Step Into Function

```
!StepIntoFunction
```

This command steps into the next function call in the current thread. It is useful
when you want to step into the implementation of a function that is called on the current line
or on an upcoming line in the current function and you want to skip over everything in between.

Here are more details about the possible options for this command:

```
StepIntoFunction - Step into a specific function call inside the current function.

Usage: !StepIntoFunction [args]

Examples:
  !StepIntoFunction                  - Step into the last function call on the current line
  !StepIntoFunction 42               - Step into the last function call on line 42
  !StepIntoFunction Initialize       - Step into the first function call with name containing "Initialize"
  !StepIntoFunction 42 Initialize    - Step into the last function call on line 42 with name containing "Initialize"
  !StepIntoFunction 42 'Initialize'  - Step into the last function call on line 42 with name containing "Initialize"
```

### Get memory

```
dq 0x00006874`00055080 L20
```

or,
```
dq 0x0000687400055080 L20
```

This command dumps 20 quadwords (64-bit values) starting from the specified
address `0x0000687400055080`.

Here is the format for the command and possible variations:

```
d{a|b|c|d|D|f|p|q|u|w|W} <address> [L<size>]
```

Where:
```
da	ASCII characters. Each line displays up to 48 characters. The display continues until the first null byte or until all characters in range have been displayed. All nonprintable characters, such as carriage returns and line feeds, are displayed as periods (.).
db	Byte values and ASCII characters. Each display line shows the address of the first byte in the line, followed by up to 16 hexadecimal byte values. The byte values are immediately followed by the corresponding ASCII values. The eighth and ninth hexadecimal values are separated by a hyphen (-). All nonprintable characters, such as carriage returns and line feeds, are displayed as periods (.). The default count is 128 bytes.
dc	Double-word values (4 bytes) and ASCII characters. Each display line shows the address of the first word in the line and up to eight hexadecimal word values and their ASCII equivalent. The default count is 32 DWORDs (128 bytes).
dd	Double-word values (4 bytes). The default count is 32 DWORDs (128 bytes).
dD	Double-precision floating-point numbers (8 bytes). The default count is 15 numbers (120 bytes).
df	Single-precision floating-point numbers (4 bytes). The default count is 16 numbers (64 bytes).
dp	Pointer-sized values. This command is equivalent to dd or dq, depending on whether the target computer processor architecture is 32-bit or 64-bit, respectively. The default count is 32 DWORDs or 16 quad-words (128 bytes).
dq	Quad-word values (8 bytes). The default count is 16 quad-words (128 bytes).
du	Unicode characters. Each line displays up to 48 characters. The display continues until the first null byte or until all characters in range have been displayed. All nonprintable characters, such as carriage returns and line feeds, are displayed as periods (.).
dw	Word values (2 bytes). Each display line shows the address of the first word in the line and up to eight hexadecimal word values. The default count is 64 words (128 bytes).
dW	Word values (2 bytes) and ASCII characters. Each display line shows the address of the first word in the line and up to eight hexadecimal word values. The default count is 64 words (128 bytes).
dyb	Binary values and byte values. The default count is 32 bytes.
dyd	Binary values and double-word values (4 bytes). The default count is 8 DWORDs (32 bytes).
```

### Get Current Process

```
dx -r2 Debugger.State.DebuggerVariables.curprocess
```

This command retrieves the current process being debugged. It provides details
about the process, including its ID, name, and other relevant information.

### List All Processes

```
|
```

This command lists all processes currently being debugged.

### Switch To Process

```
| 4 s
```

This command switches the debugger context to the specified process ID (in this
case, process ID 4).  The id is the first number in the output of the `|`
command.

### Get Process Command Line

```
!peb
```

This command retrieves the process environment block (PEB) for the current
process, which includes the command line used to start the process. The command
line can be found on the line which starts with `CommandLine:`.

## Target Details

The target being debugged is the Chromium browser, specifically the `chrome.exe`
process and the corresponding `chrome.dll`. The DLL contains the actual code
that is being debugged. This is a multi-process application.

Assume that all symbols which are part of the chromium source are located in
the `chrome.dll` module. This includes all functions, classes, and variables
that are part of the Chromium source code. The `chrome.dll` module is the main
module that contains all the code being debugged.

# WinDbg Extension Commands

This document describes the WinDbg extension commands provided by this project. These
commands extend WinDbg's functionality with improved debugging workflows, breakpoint
management and command automation.

All commands support the "?" argument which shows inline help in the command window.

## General Utility Commands

These commands provide general functionality that aid in speeding up specific
workflows.

### !StepIntoFunction

Step into a specific function call inside the current function.
When stepping into a function on a source line, there are often auxiliary
functions that get called. This command allows for skipping over these
intermediate functions and stepping straight into the desired call. It can
also be used to step into a function in the current method which comes after
the current source line.

**Usage:** `!StepIntoFunction [args]`

**Parameters:**
- `[line_number]` - Optional line number in current source file
- `[function_name]` - Optional function name or pattern to match

**Examples:**
```
!StepIntoFunction                  - Step into the last function call on current line
!StepIntoFunction 42               - Step into the last function call on line 42
!StepIntoFunction Initialize       - Step into first function call containing "Initialize"
!StepIntoFunction 42 Initialize    - Step into function call on line 42 containing "Initialize"
!StepIntoFunction 42 'Initialize'  - Step into function call on line 42 containing "Initialize"
!StepIntoFunction ?                - Show command help
```

**Note:** This command requires `continuation_commands.js` to be loaded.

### !GetCallbackLocation

Finds the actual function location for a Chrome `base::OnceCallback` or
`base::RepeatingCallback` object.

**Usage:** `!GetCallbackLocation callback [optional_command]`

**Parameters:**
- `callback` - A Chrome `base::OnceCallback` or `base::RepeatingCallback` object
- `optional_command` - Optional command to execute after finding the location:
  - `bp` - Set a breakpoint on the callback function
  - `bpg` - Set a breakpoint on the callback function and continue execution
  - `bp1` - Set a one-time breakpoint on the callback function
  - `bp1g` - Set a one-time breakpoint on the callback function and continue execution

**Examples:**
```
!GetCallbackLocation callback_object
!GetCallbackLocation task.task_.callback_ bp
!GetCallbackLocation pending_task.task_.callback_ bp1g
```

**Description:**
This command examines a Chrome callback object and determines the actual function
location that will be invoked when the callback is run. It works with both regular
callbacks and those created with `BindPostTask`. The command outputs the function
address, source location, and provides ready-to-use breakpoint commands.

**Note:** This command requires `callback_location.js` to be loaded.

### !g

Continue execution with optional line number target. The line number is
intended to be used to jump to a location further down in the current function.

**Usage:** `!g [line_number]`

**Parameters:**
- `[line_number]` - Optional line number in current source file to break at

**Examples:**
```
!g           - Continue execution (equivalent to 'g' command)
!g 42        - Set a one-time breakpoint at line 42 in current file and continue
```

**Description:**
This command continues execution. If a line number is provided, it sets a one-time
breakpoint at that line in the current source file before continuing. Note: this sets
a breakpoint for the current thread only so it works best when jumping to later parts
of the current function.

**Note:** This command requires `continuation_commands.js` to be loaded.

## Command Lists

Command lists allow you to record sequences of WinDbg commands and replay them later.
They are automatically associated with source locations and can be triggered manually
or automatically when breaking at specific locations.

### !StartCommandListRecording

Start recording a command list.

**Usage:** `!StartCommandListRecording [name] [description]`

**Parameters:**
- `[name]` - Optional name for the command list
- `[description]` - Optional description for the command list

**Examples:**
```
!StartCommandListRecording                            - Start recording without name
!StartCommandListRecording MyList                     - Start recording with name
!StartCommandListRecording MyList 'My test commands'  - Start with name and description
```

**Note:** Use single quotes around the description if it contains spaces. Use
`!StopCommandListRecording` to finish recording.

### !StopCommandListRecording

Stop recording the current command list.

**Usage:** `!StopCommandListRecording`

**Description:**
Stops recording and saves the command list.

### !PauseCommandListRecording

Pause recording commands.

**Usage:** `!PauseCommandListRecording`

**Description:**
Commands executed after this will not be included in the recording. Use
`!ResumeCommandListRecording` to continue.

### !ResumeCommandListRecording

Resume recording commands.

**Usage:** `!ResumeCommandListRecording`

**Description:**
Resumes command recording after `!PauseCommandListRecording`.

### !RunCommandList

Execute a recorded command list.

**Usage:** `!RunCommandList [option]`

**Options:**
- `(no args)` - Run the closest command list to current line
- `name` - Run the first command list with matching name
- `number` - Run command list at index 'number'
- `.:number` - Run the first command list at line 'number' in current file

**Examples:**
```
!RunCommandList          - Run closest command list to current line
!RunCommandList MyList   - Run command list with name containing 'MyList'
!RunCommandList 0        - Run command list at index 0
!RunCommandList .:123    - Run command list at line 123 in current file
```

### !ListCommandLists

List all recorded command lists.

**Usage:** `!ListCommandLists [options]`

**Options:**
- `(no args)` - List all command lists
- `text` - List command lists containing 'text' (case insensitive)
- `number` - Show detailed view of command list at index 'number'
- `.` - List all command lists in the current source file
- `s:text_pattern` - List command lists containing 'text_pattern'
- `n:text_pattern` - List command lists with names containing 'text_pattern'
- `.:number` - Show detailed view of command list at line 'number' in current file

**Examples:**
```
!ListCommandLists              - List all command lists
!ListCommandLists test         - List command lists containing 'test'
!ListCommandLists 0            - Show detailed view of command list at index 0
!ListCommandLists .            - List all command lists in current source file
!ListCommandLists s:init       - List command lists containing 'init'
!ListCommandLists n:MyList     - List command lists with names containing 'MyList'
!ListCommandLists .:123        - Show detailed view of command list at line 123
```

### !RemoveCommandList

Remove a command list from history.

**Usage:** `!RemoveCommandList <option>`

**Options:**
- `number` - Remove command list at index 'number'
- `.:number` - Remove command list at line 'number' in current file

**Examples:**
```
!RemoveCommandList 0      - Remove command list at index 0
!RemoveCommandList .:123  - Remove command list at line 123 in current file
```

### !ConvertCommandListToJavascript

Convert a command list to a JavaScript function.

**Usage:** `!ConvertCommandListToJavascript <index>`

**Parameters:**
- `<index>` - Index of the command list to convert

**Examples:**
```
!ConvertCommandListToJavascript 0  - Convert command list at index 0 to JavaScript
```

**Description:**
The generated JavaScript function will be named 'Run[CommandListName]Commands' where
[CommandListName] is the name of the command list.

### !SetCommandListAutoRun

Set the auto-run flag for a command list.

**Usage:** `!SetCommandListAutoRun <index> <value>`

**Parameters:**
- `<index>` - Index of the command list to update
- `<value>` - Auto-run setting: true, false, 1, or 0

**Examples:**
```
!SetCommandListAutoRun 0 true   - Enable auto-run for command list at index 0
!SetCommandListAutoRun 2 false  - Disable auto-run for command list at index 2
!SetCommandListAutoRun 1 1      - Enable auto-run for command list at index 1
!SetCommandListAutoRun 3 0      - Disable auto-run for command list at index 3
```

**Description:**
When auto-run is enabled, the command list will automatically execute when the
debugger breaks at the exact source location where it was created.

### !SetCommandListsFile

Set the path for the command lists file and reload command lists from the new location.

**Usage:** `!SetCommandListsFile <filePath>`

**Parameters:**
- `<filePath>` - The full path to the command lists JSON file
  - Must be a valid file path
  - The directory must exist (the file will be created if it doesn't exist)

**Examples:**
```
!SetCommandListsFile C:\Debugger\my_commands.json            - Set custom command lists file
!SetCommandListsFile D:\Projects\debug\command_lists.json    - Use project-specific commands
```

**Notes:**
- The path is not persisted between debugging sessions
- If the file doesn't exist, it will be created when command lists are saved
- If the file exists but is invalid, the command lists will be cleared
- The default location is in the same directory as the extension DLL

### !ShowNearbyCommandLists

Show command lists near the current source location.

**Usage:** `!ShowNearbyCommandLists`

**Description:**
This command is automatically called on break events when registered.
This is not intended to be called directly.

## Breakpoint History

These commands provide advanced breakpoint management with history, tagging, and
batch operations.

Note: breakpoints are only saved to history if they are created using one of the
`!Set*Breakpoint...` commands. Breakpoints set directly from within WinDbg using
either the source window or `bp ...` will not be recorded to history. This is
intentional and part of the design and helps avoid unnecessary breakpoints
being recorded to history.

### !ListBreakpointsHistory

Display the saved breakpoint lists from history.

**Usage:** `!ListBreakpointsHistory [searchTerm] [count]`

**Parameters:**
- `searchTerm` - Optional filter to limit the displayed breakpoints:
  - `null` - Shows all breakpoint lists (default)
  - `"s:term"` - Shows only breakpoint lists containing "term"
  - `"searchTerm"` - Shows only breakpoint lists containing "searchTerm"
- `count` - Optional maximum number of breakpoint lists to display:
  - Default: 15
  - 0: Shows all matching breakpoint lists
  - n: Shows at most n breakpoint lists

**Examples:**
```
!ListBreakpointsHistory                 - Show up to 15 recent breakpoint lists
!ListBreakpointsHistory 5               - Show only the first 5 breakpoint lists
!ListBreakpointsHistory ReadFile        - Show breakpoint lists containing "ReadFile"
!ListBreakpointsHistory s:ReadFile      - Show breakpoint lists containing "ReadFile"
!ListBreakpointsHistory ReadFile 5      - Show breakpoint lists containing "ReadFile" (up to 5)
!ListBreakpointsHistory 0               - Show all breakpoint lists in history
```

**Note:** The indices shown can be used with other functions like `!SetBreakpoints
<index>` or `!RemoveBreakpointsFromHistory <index>`.

### !SetBreakpoints

Set breakpoints in the current process only.

**Usage:** `!SetBreakpoints [!] [breakpointsDelimited] [newModuleName] [newTag]`

**Parameters:**

- `"!"` - Dry-run mode - shows what would be done without executing commands
- `breakpointsDelimited` - A string of breakpoint locations separated by semicolons (;) or:
  - `null` - Uses the first breakpoint in history
  - `Number` - Index of breakpoint from history to use
  - `"s:term"` - Searches breakpoint history for "term"
  - `"t:tag"` - Finds breakpoints with matching tag
  - Space-separated numbers: Combines breakpoints from specified indices (e.g., '1 2 3')
  - Space-separated numbers with .n suffix: Uses specific breakpoints (e.g., '1.0 2.1 3')
  - `'Space-separated numbers + breakpoints'` - Combines breakpoints from history with new ones
  - `"."` - Set breakpoint at the current source location
  - `".:line"` - Set breakpoint at specified line number in the current source file
- `newModuleName` - The module to set breakpoints in:
  - `null` - Uses default if creating new, or original if from history
  - `"."` - Uses the original module name from history
  - Any other string: Updates the module name
- `newTag` - A descriptive tag for these breakpoints:
  - `null` - Uses original tag if from history
  - `"-"` - Uses empty string (clears the tag)

**Examples:**
```
!SetBreakpoints                                         - Use first breakpoint from history
!SetBreakpoints 3                                       - Use breakpoint at index 3 from history
!SetBreakpoints 3.1                                     - Use second breakpoint at index 3
!SetBreakpoints ! 3                                     - Show what would be done for index 3
!SetBreakpoints 3 tests.exe                             - Use index 3, update module name
!SetBreakpoints 3 . new_tag                             - Use index 3, set new tag
!SetBreakpoints 3 . -                                   - Use index 3, remove tag
!SetBreakpoints '1 2 3'                                 - Combine breakpoints from indices 1,2,3
!SetBreakpoints '1.0 2.1'                               - Use first from list 1, second from list 2
!SetBreakpoints s:ReadFile                              - Search history for "ReadFile"
!SetBreakpoints t:file_operations                       - Find breakpoints with tag "file_operations"
!SetBreakpoints kernel32!ReadFile kernel32.dll file_ops - Set new breakpoint
!SetBreakpoints 'kernel32!ReadFile; kernel32!WriteFile' kernel32.dll file_ops - Set new breakpoints
!SetBreakpoints '1 2.1 4 + chrome!ReadFile'            - Combine history with new breakpoint
!SetBreakpoints '0 + ntdll!NtCreateFile; ntdll!NtReadFile' - Add new to history index 0
!SetBreakpoints .                                       - Set breakpoint at current location
!SetBreakpoints .:150                                   - Set breakpoint at line 150
```

**Note:** If an input argument contains a space then it needs to be enclosed in single
quotes. Unlike `!SetAllProcessesBreakpoints`, this function only affects the current
process.

### !SetAllProcessesBreakpoints

Set breakpoints that will be applied to all processes, including child processes that
are launched after the breakpoints are set.

**Usage:** `!SetAllProcessesBreakpoints [!] [breakpointsDelimited] [newModuleName] [newTag]`

**Parameters:** Same as `!SetBreakpoints`

**Examples:**
```
!SetAllProcessesBreakpoints                             - Use first breakpoint from history
!SetAllProcessesBreakpoints 3                          - Use breakpoint at index 3
!SetAllProcessesBreakpoints 3.1                        - Use second breakpoint at index 3
!SetAllProcessesBreakpoints ! 3                        - Show what would be done for index 3
!SetAllProcessesBreakpoints s:ReadFile                 - Use first from history matching "ReadFile"
!SetAllProcessesBreakpoints t:file_operations          - Use first from history matching tag "file_operations"
!SetAllProcessesBreakpoints '1 2 3'                    - Combine breakpoints from indices
!SetAllProcessesBreakpoints '1 2.1 4 + chrome!ReadFile' - Combine history with new breakpoint
```

### !RemoveBreakpointsFromHistory

Remove specific breakpoint lists from the saved history.

**Usage:** `!RemoveBreakpointsFromHistory <input>`

**Parameters:**
- `input` - Specifies which breakpoint lists to remove:
  - `Number` - Index of a single breakpoint list to remove
  - `"0 1 2"` - Space-separated string of indices to remove multiple lists
  - `"0-2"` - Range notation to specify consecutive indices (equivalent to "0 1 2")
  - `"1-3 5 7-9"` - Mixed format combining ranges and individual indices
  - `"s:term"` - Removes all breakpoint lists matching the search term
  - `"t:tag"` - Removes all breakpoint lists with tags matching the given tag

**Examples:**
```
!RemoveBreakpointsFromHistory 0                 - Remove breakpoint list at index 0
!RemoveBreakpointsFromHistory '3 5 7'           - Remove lists at indices 3, 5, and 7
!RemoveBreakpointsFromHistory 1-5               - Remove lists at indices 1, 2, 3, 4, and 5
!RemoveBreakpointsFromHistory '1-3 5 7-9'       - Remove lists at specified indices
!RemoveBreakpointsFromHistory s:ReadFile        - Remove all lists containing "ReadFile"
!RemoveBreakpointsFromHistory t:file_operations - Remove all lists with matching tags
```

**Note:** This operation cannot be undone.

### !SetBreakpointsHistoryTags

Update the tags of specific breakpoint lists in the saved history.

**Usage:** `!SetBreakpointsHistoryTags <input> <newTag>`

**Parameters:**
- `input` - Specifies which breakpoint lists to update (same format as RemoveBreakpointsFromHistory)
- `newTag` - The new tag to assign to the selected breakpoint lists:
  - String: Any text to use as the new tag
  - `"-"` - Removes existing tags (sets to empty string)

**Examples:**
```
!SetBreakpointsHistoryTags 0 new_tag              - Update tag of list at index 0
!SetBreakpointsHistoryTags '3 5 7' file_ops       - Update tags for lists at indices 3,5,7
!SetBreakpointsHistoryTags '1-5' file_ops         - Update tags for lists at indices 1-5
!SetBreakpointsHistoryTags s:ReadFile io_operations - Update tags for all lists containing "ReadFile"
!SetBreakpointsHistoryTags t:old_tag new_tag      - Update all lists with tags matching "old_tag"
!SetBreakpointsHistoryTags '0 1' -                - Remove tags from lists at indices 0 and 1
```

### !UpdateBreakpointLineNumber

Update the line number of a specific breakpoint in the history.

**Usage:** `!UpdateBreakpointLineNumber [+] <input> <newLineNumber>`

**Parameters:**
- `[+]` - Optional first argument. If present, adds the updated breakpoint as a new entry.
  Without '+', updates the breakpoint list in place.
- `input` - Specifies which breakpoint to update:
  - `number` - Index of a breakpoint list in history (updates first line-number breakpoint found)
  - `number1.number2` - Updates specific breakpoint at index number2 in list at index number1
- `newLineNumber` - The new line number to set (must be a positive integer)

**Examples:**
```
!UpdateBreakpointLineNumber 0 150       - Update in place first source:line breakpoint in list 0
!UpdateBreakpointLineNumber + 0 150     - Update first source:line breakpoint and add as new entry
!UpdateBreakpointLineNumber 2.1 275     - Update in place second breakpoint in list 2 to line 275
!UpdateBreakpointLineNumber + 2.1 275   - Update second breakpoint second list and add as new entry
```

**Notes:**
- This only works for breakpoints that include line numbers (e.g., "module!file.cpp:123")
- With '+': The updated breakpoint list will be saved to history as a new entry
- Without '+': The breakpoint list will be updated in place
- The updated breakpoint will NOT be automatically set in the debugger

### !SetBreakpointListsFile

Set the path for the breakpoints history file and reload breakpoints from the new location.

**Usage:** `!SetBreakpointListsFile <filePath>`

**Parameters:**
- `<filePath>` - The full path to the breakpoints history JSON file
  - Must be a valid file path
  - The directory must exist (the file will be created if it doesn't exist)

**Examples:**
```
!SetBreakpointListsFile C:\Debugger\my_breakpoints.json             - Set a custom breakpoints file
!SetBreakpointListsFile D:\Projects\debug\breakpoints_history.json  - Use project-specific breakpoints
```

**Notes:**
- The path is not persisted between debugging sessions
- If the file doesn't exist, it will be created when breakpoints are saved
- If the file exists but is invalid, the breakpoints list will be cleared
- The default location is in the same directory as the extension DLL

## Break Event Commands

These commands allow you to automatically execute WinDbg commands whenever the debugger
breaks execution. This includes scenarios where the target is suspended after
commands like `p` and `t`.

### !AddBreakCommand

Adds a command to execute when the debugger breaks.

**Usage:** `!AddBreakCommand <command>`

**Parameters:**
- `<command>` - WinDbg command to execute when debugger breaks

**Examples:**
```
!AddBreakCommand k        - Executes 'k' (stack trace) at every break
!AddBreakCommand 'r eax'  - Executes 'r eax' (display eax register) at every break
!AddBreakCommand '!peb'   - Executes '!peb' at every break
```

**Description:**
This includes breaking in from commands like 'p' and 't' (step over/into). Use
`!ListBreakCommands` to see all commands and `!RemoveBreakCommands` to remove commands.

### !ListBreakCommands

List all currently registered break commands.

**Usage:** `!ListBreakCommands`

**Description:**
Shows all commands that will be executed when the debugger breaks, along with their
indices for use with `!RemoveBreakCommands`.

### !RemoveBreakCommands

Remove break commands.

**Usage:** `!RemoveBreakCommands [index]`

**Parameters:**
- `[index]` - Optional index of command to remove

**Examples:**
```
!RemoveBreakCommands     - Removes all break commands
!RemoveBreakCommands 0   - Removes only the first break command
!RemoveBreakCommands 2   - Removes the break command at index 2
```

**Description:**
Use `!ListBreakCommands` to see all existing commands with their indices.

## Command Logging

These commands allow you to record WinDbg commands for later analysis or
replay. This is mostly just used by other extensions in this project.
See `Command Lists` below and `command_lists.cpp` for an example of how
this can be used from within an extension.

### !StartCommandLogging

Begin logging WinDbg commands to a file.

**Usage:** `!StartCommandLogging <filename>`

**Parameters:**
- `<filename>` - Path to the log file (required)

**Examples:**
```
!StartCommandLogging c:\temp\windbg_commands.log
```

**Note:** Commands are appended to the file if it already exists. Use
`!StopCommandLogging` to end the logging session.

### !StopCommandLogging

End the current command logging session.

**Usage:** `!StopCommandLogging`

**Description:**
This command takes no parameters. Stops the active logging session started with
`!StartCommandLogging`.

## MCP Server

These commands provide Model Context Protocol (MCP) server functionality, allowing
AI assistants and other tools to interact with WinDbg through a JSON-RPC protocol.

### !StartMCPServer

Start an MCP (Model Context Protocol) server for AI-assisted debugging.

**Usage:** `!StartMCPServer [port]`

**Parameters:**
- `[port]` - Optional TCP port to listen on (default: 0 for automatic port selection)

**Examples:**
```
!StartMCPServer        - Start on automatic port
!StartMCPServer 8080   - Start on port 8080
```

**Description:**
The MCP server allows AI assistants and other tools to interact with WinDbg through
a JSON-RPC protocol. Once started, you can connect using `localhost:[port]`
or through `stdio` using the provided bridge application.

**Note**: this is pure sockets based implementation and does not work with `http`.
To use this server with applications like Visual Studio Code, use the bridge
application (see `build_output\mcp_stdio_bridge.exe`).

**Available MCP methods:**
- `executeCommand` - Execute a debugger command asynchronously
- `getOperationStatus` - Check async operation status
- `getDebuggerState` - Get current debugger state

**Note:** This is an experimental feature.

### !StopMCPServer

Stop the running MCP server.

**Usage:** `!StopMCPServer`

**Description:**
This command stops the MCP server if it is currently running. No parameters are
required.

### !MCPServerStatus

Show the current status of the MCP server.

**Usage:** `!MCPServerStatus`

**Description:**
Displays whether the MCP server is running, stopped, or not initialized. If running,
shows the port number and connection information.

**Example output:**
```
MCP server is running on port 8080
Connect using: tcp://localhost:8080
```

# MCP Server for WinDbg - AI Client Instructions

## Overview
This MCP server bridges communication with WinDbg. Connect using the MCP protocol.

## Available Tools

### executeCommand
Executes a WinDbg command and returns the output.

**Parameters:**
- `command` (string, required): The WinDbg command to execute

**Returns:** The command output including:
- Original command with prompt
- Command results
- New prompt
- Context information (for continuation commands: `g`, `p`, `t`, `gu`)

**Example output:**
```
5:096> k
 # Child-SP          RetAddr               Call Site
00 00000027`bddff608 00007ffe`c7d0e123     chrome!media::MediaFoundationService::IsKeySystemSupported+0x18
...

5:096>
```

### getDebuggerState
Gets the current debugger state.

**Parameters:** None

**Returns:** One of: "break", "running", "stepping", "no_debuggee", or an error

## Critical Workflow Requirements

**ALWAYS** follow this workflow:
1. Check debugger state with `getDebuggerState`
2. Only execute commands when state is "break"
3. If not "break", wait and retry (return to step 1)

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

### List Existing Breakpoints

```
bl
```

The bl command lists information about existing breakpoints.

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

If the value of the variable is `<value unavailable>` then it may be
necessary to "Step Over" one or more times using the `p` command.
One scenario where this always happens is at the start of a function
since WinDbg won't have the necessary context until it has stepped over once.

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

If the value of the variable is not availble, it may be necessary to "Step Over"
one or more times using the `p` command.
One scenario where this always happens is at the start of a function
since WinDbg won't have the necessary context until it has stepped over once.

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

### Step Out (Go Up)

```
gu
```

The gu command causes the target to execute until the current function is complete.
This can be used to step out of functions and return to the caller.

### Continue Execution (Go)

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

Another option for getting the process command line is to use:

```
dx -r0 @$curprocess.Environment.EnvironmentBlock.ProcessParameters->CommandLine.Buffer
```

This will retrieve the command line directly without having to parse the PEB output.

### Unassemble Function

```
uf Address
```

```
uf chrome!media::MediaFoundationService::IsKeySystemSupported
```

The `uf` command displays an assembly translation of the specified function in memory.

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

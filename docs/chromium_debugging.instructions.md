# Chromium Debugging Guide for AI Agents

## Overview

The target being debugged is the Chromium browser, specifically the `chrome.exe`
process and the corresponding `chrome.dll`. The DLL contains the actual code
that is being debugged. This is a multi-process application.

Assume that all symbols which are part of the chromium source are located in
the `chrome.dll` module. This includes all functions, classes, and variables
that are part of the Chromium source code. The `chrome.dll` module is the main
module that contains all the code being debugged.

## Chromium Architecture

### Multi-Process Model

Chromium uses a multi-process architecture for security and stability:

1. **Browser Process** (main process)
   - UI management
   - Tab and window management
   - Network requests coordination
   - Plugin and extension management
   - Coordinates all other processes

2. **Renderer Processes** (one per site/tab)
   - Runs Blink rendering engine
   - Executes JavaScript via V8
   - Handles DOM and CSS
   - Sandboxed for security

3. **GPU Process**
   - Hardware acceleration
   - WebGL and graphics compositing
   - Video decoding

4. **Utility Processes**
   - Audio/video decoding
   - Network services
   - Storage services
   - Media (Content Decryption Module) for DRM

5. **Plugin Processes**
   - PPAPI plugins
   - Flash (legacy)

### Process Types in Command Line

When debugging, identify process type from command line:
- Browser: No `--type` flag
- Renderer: `--type=renderer`
- GPU: `--type=gpu-process`
- Utility: `--type=utility` (check `--utility-sub-type` for specifics)
- Network: `--type=utility --utility-sub-type=network.mojom.NetworkService`
- Audio: `--type=utility --utility-sub-type=audio.mojom.AudioService`
- Media: `--type=utility --utility-sub-type=media.mojom.MediaFoundationServiceBroker`

## Key Namespaces and Components

### Core Namespaces

1. **`content::`** - Browser/renderer abstraction layer
   - `content::BrowserContext` - Per-profile browser state
   - `content::RenderProcessHost` - Browser-side renderer representation
   - `content::WebContents` - Tab abstraction

2. **`chrome::`** - Chrome-specific features
   - `chrome::ChromeBrowserMainParts`
   - Chrome-specific UI and features

3. **`media::`** - Media playback and DRM
   - `media::MediaFoundationCdm` - Windows DRM implementation
   - `media::CdmFactory` - CDM creation
   - `media::ContentDecryptionModule` - DRM interface
   - `media::KeySystemProperties` - DRM capabilities

4. **`mojo::`** - Inter-process communication
   - `mojo::Message` - IPC messages
   - `mojo::InterfacePtr<>` - Client-side interface
   - `mojo::Binding<>` - Service-side binding
   - `mojo::Remote<>` - Modern client handle
   - `mojo::Receiver<>` - Modern service handle

5. **`blink::`** - Rendering engine (WebKit fork)
   - `blink::WebFrame` - Frame representation
   - `blink::WebDocument` - Document object

6. **`base::`** - Common utilities
   - `base::Callback` - Callback mechanisms
   - `base::OnceCallback` - Single-use callbacks
   - `base::RepeatingCallback` - Multi-use callbacks
   - `base::SequencedTaskRunner` - Task scheduling

7. **Anonymous namespaces** - Common in Chromium
   - Functions/classes without external linkage
   - Use source:line format for breakpoints

### Common Patterns

1. **Callback-based Async Operations**
   ```cpp
   void DoSomethingAsync(base::OnceCallback<void(bool)> callback);
   ```

2. **Mojo IPC Pattern**
   ```
   // Interface definition (defined in *.mojom files)
   interface MediaFoundationService {
     IsKeySystemSupported(string key_system)
       => (bool is_supported, KeySystemCapability key_system_capability);

   // Client usage
   mf_service_->IsKeySystemSupported(key_system,
     base::BindOnce(&Class::OnResult, weak_ptr_));
   ```

3. **Smart Pointers**
   - `std::unique_ptr<>` - Single ownership
   - `scoped_refptr<>` - Reference counted
   - `base::WeakPtr<>` - Weak references

## Additional WinDbg Debugging Commands Specific to Chromium

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

This command retrieves the location of a `OnceCallback` or `RepeatingCallback` function
in the Chromium codebase. The result includes the address of the callback function,
the symbol name, and the source file and line number where the callback is defined.

To set a breakpoint at the callback location, use the following format of the command:

```
!GetCallbackLocation callback bp1
!GetCallbackLocation callback bp1g
```

Where the `bp1` command sets a breakpoint at the callback location and the
`bp1g` command sets a breakpoint and continues execution.

## Debugging Workflows

### Setting Initial Breakpoints

Use something similar to the following command to set breakpoints on key functions
when starting a new debugging session. This is required since some of the breakpoints
will only be hit in child processes.

```
.childdbg 1; sxn ibp; sxn epr; sxe -c "bp chrome!media::CreateMediaFoundationCdm; bp chrome!media::MediaFoundationService::IsKeySystemSupported; gc" ld:chrome.dll; g
```

```
.childdbg 1; sxn ibp; sxn epr; sxe -c "bp `chrome!D:\\\\path\\\\to\\\\file\\\\source_file.cc:42`; gc" ld:chrome.dll; g
```

After the initial breakpoints are set and the application has been started,
breakpoints can be set using the normal `bp` command.

### Identifying Process Type

1. Check command line:
   ```
   dx -r0 @$curprocess.Environment.EnvironmentBlock.ProcessParameters->CommandLine.Buffer
   ```

### Callback Tracing

- Use `!GetCallbackLocation` on callback parameters
- Set breakpoints on callback execution

## Important Chromium Conventions

1. **String Types**
   - `std::string` - UTF-8 strings
   - `std::u16string` - UTF-16 strings
   - `base::StringPiece` - Non-owning string view

2. **Threading**
   - UI thread (browser process main thread)
   - IO thread (network operations)
   - Task runners for specific sequences

3. **Error Handling**
   - `HRESULT` for Windows APIs
   - `bool` returns with callbacks for async errors
   - `base::Optional<>` for optional values

4. **Logging**
   - `LOG(INFO)`, `LOG(ERROR)` - General logging
   - `DVLOG(1)` - Verbose debug logging
   - `DCHECK()` - Debug assertions

## Common Pitfalls

1. **Wrong Process**
   - Always verify you're in the correct process type
   - Use `|` to list all processes
   - Switch with `| <id> s`

2. **Anonymous Namespace Symbols**
   - Can't set breakpoints by name
   - Use source:line format instead. Make sure to include the `chrome!` module
     prefix when setting source:line breakpoints: `bp chrome!anonymous_namespace::FunctionName`

3. **Optimized Code**
   - Local variables may be `<value unavailable>`
   - Step once with `p` to get context
   - Some values may be in registers. The `uf` command can be used to
     disassemble the function and see register usage.

4. **Async Operations**
   - Results come via callbacks
   - Trace callback setup and execution separately
   - Use `!GetCallbackLocation` to find callback locations
     and set breakpoints when callbacks are executed
   - Check weak pointer validity

Remember: Chromium is heavily asynchronous. Most operations complete via
callbacks, so debugging often requires setting breakpoints at both the request
and response points. The `!GetCallbackLocation` can be helpful in setting
breakpoints on callback execution.

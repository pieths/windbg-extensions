// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

// This file wraps JavaScript methods with native WinDbg extension commands.
// JavaScript methods can be exposed directly from the script file using
// `host.functionAlias`. Here is an example of what that would look like
// for the `StepIntoFunction` method:
//
//   function initializeScript()
//   {
//       return [
//             new host.functionAlias(__StepIntoFunction, "StepIntoFunction"),
//       ];
//    }
//
// But exposing methods in this way has its drawbacks. The biggest one being
// the way that arguments are passed when using the !command_name syntax.
// For JavaScript backed commands, multiple arguments need to be separated by
// commas, and text based arguments that are not referring to the symbols in the
// current context need to be quoted with double quotes. This conflicts with the
// way that arguments are passed with native extensions.
//
// This file wraps the JavaScript methods so that they can be called using the
// native WinDbg extension command syntax, which allows for more flexible
// argument parsing and does not require double quotes for text arguments.
// This also provides a more consistent user experience across all the different
// commands that are provided in this project.
#include <dbgeng.h>
#include <windows.h>
#include <string>

#include "utils.h"

utils::DebugInterfaces g_debug;

std::string BuildStepIntoFunctionCommand(const char* args) {
  std::string jsCommand =
      "dx Debugger.State.Scripts.continuation_commands.Contents.StepIntoFunction(";
  std::vector<std::string> parsed_args = utils::ParseCommandLine(args);

  if (parsed_args.empty()) {
    // No arguments provided, call without params
    jsCommand += ")";
    return jsCommand;
  }

  if (parsed_args.size() > 2) {
    DERROR(
        "Error: Too many arguments provided. Expected 0, 1, or 2 arguments.\n");
    return "";
  }

  std::string first_arg = utils::Trim(parsed_args[0]);

  if (utils::IsWholeNumber(first_arg)) {
    // If the first argument is a number, use it as the line number
    jsCommand += first_arg;

    if (parsed_args.size() == 2) {
      // If a second argument is provided, treat it as a function name or
      // pattern
      std::string second_arg = utils::Trim(parsed_args[1]);
      if (second_arg.empty()) {
        DERROR("Error: Second argument cannot be empty.\n");
        return "";
      }

      jsCommand += ", \"" + second_arg + "\"";
    }
  } else {
    // Otherwise, treat it as a function name or pattern
    jsCommand += "\"" + first_arg + "\"";
  }

  jsCommand += ")";
  return jsCommand;
}

HRESULT CALLBACK StepIntoFunctionInternal(IDebugClient* client,
                                          const char* args) {
  if (args && args[0] == '?' && args[1] == '\0') {
    DOUT(
        "StepIntoFunction - Step into a specific function call inside the current function.\n\n"
        "Usage: !StepIntoFunction [args]\n\n"
        "Examples:\n"
        "  !StepIntoFunction                  - Step into the last function call on the current line\n"
        "  !StepIntoFunction 42               - Step into the last function call on line 42\n"
        "  !StepIntoFunction Initialize       - Step into the first function call with name containing \"Initialize\"\n"
        "  !StepIntoFunction 42 Initialize    - Step into the last function call on line 42 with name containing \"Initialize\"\n"
        "  !StepIntoFunction 42 'Initialize'  - Step into the last function call on line 42 with name containing \"Initialize\"\n"
        "  !StepIntoFunction ?                - Show command help\n\n"
        "This command requires continuation_commands.js to be loaded.\n\n");
    return S_OK;
  }

  std::string jsCommand = BuildStepIntoFunctionCommand(args);
  if (!jsCommand.empty()) {
    g_debug.control->Execute(DEBUG_OUTCTL_ALL_CLIENTS, jsCommand.c_str(),
                             DEBUG_EXECUTE_DEFAULT);
  }
  return S_OK;
}

std::string BuildGetCallbackLocationCommand(const char* args) {
  std::string jsCommand =
      "dx Debugger.State.Scripts.callback_location.Contents.GetCallbackLocation(";
  std::vector<std::string> parsed_args = utils::ParseCommandLine(args);

  if (parsed_args.empty() || parsed_args[0].empty()) {
    DERROR("Error: No arguments provided. Expected 1 or 2 arguments.\n");
    return "";
  }

  if (parsed_args.size() > 2) {
    DERROR("Error: Too many arguments provided. Expected 1 or 2 arguments.\n");
    return "";
  }

  // The first argument is the callback object.
  // Don't quote the callback object name in the dx command
  // so that the debugger passes the object reference directly
  // to the JavaScript function.
  std::string callback_arg = utils::Trim(parsed_args[0]);
  jsCommand += callback_arg;

  // Add optional command if provided
  if (parsed_args.size() == 2) {
    std::string command_arg = utils::Trim(parsed_args[1]);
    if (command_arg.empty()) {
      g_debug.control->Output(DEBUG_OUTPUT_ERROR,
                              "Error: Second argument cannot be empty.\n");
      return "";
    }

    jsCommand += ", \"" + command_arg + "\"";
  }

  jsCommand += ")";
  return jsCommand;
}

HRESULT CALLBACK GetCallbackLocationInternal(IDebugClient* client,
                                             const char* args) {
  if (args && args[0] == '?' && args[1] == '\0') {
    DOUT(
        "GetCallbackLocation - Finds the actual function location for a Chrome base::OnceCallback or base::RepeatingCallback\n\n"
        "Usage: !GetCallbackLocation callback [optional_command]\n\n"
        "Parameters:\n"
        "  callback        - A Chrome base::OnceCallback or base::RepeatingCallback object\n"
        "  optional_command - Optional command to execute after finding the location:\n"
        "                     bp   - Set a breakpoint on the callback function\n"
        "                     bpg  - Set a breakpoint and continue execution\n"
        "                     bp1  - Set a one-time breakpoint\n"
        "                     bp1g - Set a one-time breakpoint and continue execution\n\n"
        "Examples:\n"
        "  !GetCallbackLocation callback_object\n"
        "  !GetCallbackLocation task.task_.callback_ bp\n"
        "  !GetCallbackLocation pending_task.task_.callback_ bp1g\n\n"
        "Description:\n"
        "  This command examines a Chrome callback object and determines the actual\n"
        "  function location that will be invoked when the callback is run. It works\n"
        "  with both regular callbacks and those created with BindPostTask.\n"
        "  The command outputs the function address, source location, and provides\n"
        "  ready-to-use breakpoint commands.\n\n"
        "This command requires callback_location.js to be loaded.\n\n");
    return S_OK;
  }

  std::string jsCommand = BuildGetCallbackLocationCommand(args);
  if (!jsCommand.empty()) {
    g_debug.control->Execute(DEBUG_OUTCTL_ALL_CLIENTS, jsCommand.c_str(),
                             DEBUG_EXECUTE_DEFAULT);
  }
  return S_OK;
}

std::string BuildGoCommand(const char* args) {
  std::string jsCommand =
      "dx Debugger.State.Scripts.continuation_commands.Contents.Go(";
  std::vector<std::string> parsed_args = utils::ParseCommandLine(args);

  if (parsed_args.empty()) {
    // No arguments provided, call without params
    jsCommand += ")";
    return jsCommand;
  }

  if (parsed_args.size() > 1) {
    DERROR("Error: Too many arguments provided. Expected 0 or 1 argument.\n");
    return "";
  }

  std::string first_arg = utils::Trim(parsed_args[0]);

  if (utils::IsWholeNumber(first_arg)) {
    // If the argument is a number, pass it as a number
    jsCommand += first_arg;
  } else {
    DERROR("Error: Invalid argument. Expected a line number.\n");
    return "";
  }

  jsCommand += ")";
  return jsCommand;
}

HRESULT CALLBACK GoInternal(IDebugClient* client, const char* args) {
  if (args && args[0] == '?' && args[1] == '\0') {
    DOUT(
        "Go - Continue execution with optional line number target\n\n"
        "Usage: !g [line_number]\n\n"
        "Parameters:\n"
        "  [line_number] - Optional line number in current source file to break at\n\n"
        "Examples:\n"
        "  !g           - Continue execution (equivalent to 'g' command)\n"
        "  !g 42        - Set a one-time breakpoint at line 42 in current file and continue.\n"
        "                 Note, this sets a breakpoint for the current thread only so it\n"
        "                 works best when jumping to later parts of the current function.\n\n"
        "Description:\n"
        "  This command continues execution. If a line number is provided, it sets\n"
        "  a one-time breakpoint at that line in the current source file before continuing.\n\n"
        "This command requires continuation_commands.js to be loaded.\n\n");
    return S_OK;
  }

  std::string jsCommand = BuildGoCommand(args);
  if (!jsCommand.empty()) {
    g_debug.control->Execute(DEBUG_OUTCTL_ALL_CLIENTS, jsCommand.c_str(),
                             DEBUG_EXECUTE_DEFAULT);
  }
  return S_OK;
}

HRESULT CALLBACK DebugExtensionInitializeInternal(PULONG version,
                                                  PULONG flags) {
  *version = DEBUG_EXTENSION_VERSION(1, 0);
  *flags = 0;
  return utils::InitializeDebugInterfaces(&g_debug);
}

HRESULT CALLBACK DebugExtensionUninitializeInternal() {
  return utils::UninitializeDebugInterfaces(&g_debug);
}

extern "C" {
__declspec(dllexport) HRESULT CALLBACK DebugExtensionInitialize(PULONG version,
                                                                PULONG flags) {
  return DebugExtensionInitializeInternal(version, flags);
}

__declspec(dllexport) HRESULT CALLBACK DebugExtensionUninitialize(void) {
  return DebugExtensionUninitializeInternal();
}

__declspec(dllexport) HRESULT CALLBACK StepIntoFunction(IDebugClient* client,
                                                        const char* args) {
  return StepIntoFunctionInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK GetCallbackLocation(IDebugClient* client,
                                                           const char* args) {
  return GetCallbackLocationInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK g(IDebugClient* client,
                                         const char* args) {
  return GoInternal(client, args);
}
}

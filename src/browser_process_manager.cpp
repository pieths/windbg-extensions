// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#include <dbgeng.h>
#include <windows.h>
#include <string>
#include <vector>

#include "utils.h"

utils::DebugInterfaces g_debug;

HRESULT CALLBACK DebugExtensionInitializeInternal(PULONG version,
                                                  PULONG flags) {
  *version = DEBUG_EXTENSION_VERSION(1, 0);
  *flags = 0;
  return utils::InitializeDebugInterfaces(&g_debug);
}

HRESULT CALLBACK DebugExtensionUninitializeInternal() {
  return utils::UninitializeDebugInterfaces(&g_debug);
}

HRESULT CALLBACK CommandLineInternal(IDebugClient* client, const char* args) {
  if (args && *args && (args[0] == '?' && args[1] == '\0')) {
    DOUT(
        "CommandLine - Gets the command line of the current process.\n\n"
        "Usage: !CommandLine\n\n"
        "This command displays the command line arguments used to start\n"
        "the current process being debugged.\n");
    return S_OK;
  }

  try {
    // Get the Process Environment Block (PEB) to access command line
    std::string peb_command = "!peb";
    std::string peb_output = utils::ExecuteCommand(&g_debug, peb_command);
    
    if (peb_output.empty()) {
      DOUT("Error: Failed to get PEB information.\n");
      return E_FAIL;
    }
    
    // Look for CommandLine in the PEB output
    size_t cmd_line_pos = peb_output.find("CommandLine:");
    if (cmd_line_pos == std::string::npos) {
      DOUT("Error: Could not find CommandLine in PEB output.\n");
      return E_FAIL;
    }
    
    // Extract the command line portion
    size_t start_pos = peb_output.find("'", cmd_line_pos);
    if (start_pos == std::string::npos) {
      DOUT("Error: Could not parse CommandLine from PEB output.\n");
      return E_FAIL;
    }
    start_pos++; // Skip the opening quote
    
    size_t end_pos = peb_output.find("'", start_pos);
    if (end_pos == std::string::npos) {
      DOUT("Error: Could not find end of CommandLine in PEB output.\n");
      return E_FAIL;
    }
    
    std::string command_line = peb_output.substr(start_pos, end_pos - start_pos);
    
    DOUT("Command Line: %s\n", command_line.c_str());
    
  } catch (const std::exception& e) {
    DOUT("Error retrieving command line: %s\n", e.what());
    return E_FAIL;
  } catch (...) {
    DOUT("Unknown error retrieving command line.\n");
    return E_FAIL;
  }
  
  return S_OK;
}

extern "C" {

__declspec(dllexport) HRESULT CALLBACK DebugExtensionInitialize(PULONG version, PULONG flags) {
  return DebugExtensionInitializeInternal(version, flags);
}

__declspec(dllexport) HRESULT CALLBACK DebugExtensionUninitialize() {
  return DebugExtensionUninitializeInternal();
}

__declspec(dllexport) HRESULT CALLBACK CommandLine(IDebugClient* client, const char* args) {
  return CommandLineInternal(client, args);
}

}  // extern "C"

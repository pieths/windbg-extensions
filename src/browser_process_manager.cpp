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
  if (!args || !*args || (args[0] == '?' && args[1] == '\0')) {
    DOUT(
        "CommandLine - Manages browser process command line operations.\n\n"
        "Usage: !CommandLine <operation> [parameters]\n\n"
        "  <operation>  - Operation to perform (not yet implemented)\n"
        "  [parameters] - Optional parameters for the operation\n\n"
        "Note: This command is not yet implemented.\n");
    return S_OK;
  }

  // TODO: Implement command line functionality
  DOUT("CommandLine command is not yet implemented.\n");
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

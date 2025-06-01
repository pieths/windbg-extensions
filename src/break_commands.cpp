// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#include <dbgeng.h>
#include <windows.h>
#include <string>
#include <vector>

#include "utils.h"

utils::DebugInterfaces g_debug;

// The commands to be executed when the target is suspended
std::vector<std::string> g_commands;

class BreakEventHandler;
BreakEventHandler* g_break_event_handler = nullptr;

class BreakEventHandler : public DebugBaseEventCallbacks {
 public:
  BreakEventHandler() : m_refCount(1) {}

  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID InterfaceId,
                                           PVOID* Interface) override {
    *Interface = NULL;
    if (IsEqualIID(InterfaceId, __uuidof(IUnknown)) ||
        IsEqualIID(InterfaceId, __uuidof(IDebugEventCallbacks))) {
      *Interface = (IDebugEventCallbacks*)this;
      AddRef();
      return S_OK;
    }
    return E_NOINTERFACE;
  }

  ULONG STDMETHODCALLTYPE AddRef() override {
    return InterlockedIncrement(&m_refCount);
  }

  ULONG STDMETHODCALLTYPE Release() override {
    ULONG count = InterlockedDecrement(&m_refCount);
    if (count == 0) {
      delete this;
    }
    return count;
  }

  STDMETHODIMP BreakEventHandler::GetInterestMask(PULONG mask) {
    *mask = DEBUG_EVENT_CHANGE_ENGINE_STATE;
    return S_OK;
  }

  STDMETHODIMP ChangeEngineState(ULONG flags, ULONG64 argument) {
    if (flags & DEBUG_CES_EXECUTION_STATUS) {
      // Check if the target was suspended.
      // DEBUG_STATUS_BREAK indicates that the target is suspended
      // for any reason. This includes things like step into and step over.
      if (argument == DEBUG_STATUS_BREAK && !g_commands.empty()) {
        // Execute all registered commands
        for (const auto& cmd : g_commands) {
          g_debug.control->Execute(DEBUG_OUTPUT_NORMAL, cmd.c_str(),
                                   DEBUG_EXECUTE_DEFAULT);
        }
      }
    }
    return S_OK;
  }

 private:
  LONG m_refCount;
};

HRESULT CALLBACK DebugExtensionInitializeInternal(PULONG version,
                                                  PULONG flags) {
  *version = DEBUG_EXTENSION_VERSION(1, 0);
  *flags = 0;

  return utils::InitializeDebugInterfaces(&g_debug);
}

HRESULT CALLBACK DebugExtensionUninitializeInternal() {
  if (g_break_event_handler) {
    g_debug.client->SetEventCallbacks(nullptr);
    g_break_event_handler->Release();
    g_break_event_handler = nullptr;
  }

  return utils::UninitializeDebugInterfaces(&g_debug);
}

HRESULT CALLBACK AddBreakCommandInternal(IDebugClient* client,
                                         const char* args) {
  if (!args || !*args || (args[0] == '?' && args[1] == '\0')) {
    DOUT(
        "AddBreakCommand - Adds a command to execute when the debugger breaks.\n"
        "                  This includes breaking in from commands like 'p' and 't' (step over/into).\n\n"
        "Usage: !AddBreakCommand <command>\n\n"
        "  <command>  - WinDbg command to execute when debugger breaks\n\n"
        "Examples:\n"
        "  !AddBreakCommand k        - Executes 'k' (stack trace) at every break\n"
        "  !AddBreakCommand 'r eax'  - Executes 'r eax' (display eax register) at every break\n"
        "  !AddBreakCommand '!peb'   - Executes '!peb' at every break\n\n"
        "Use !ListBreakCommands to see all commands\n"
        "Use !RemoveBreakCommands to remove commands\n");
    return S_OK;
  }

  // Add the new command to the list
  g_commands.push_back(args);
  DOUT("Break command added: %s\n", args);

  if (!g_break_event_handler) {
    g_break_event_handler = new BreakEventHandler();
    HRESULT hr = g_debug.client->SetEventCallbacks(g_break_event_handler);
    if (FAILED(hr)) {
      g_break_event_handler->Release();
      g_break_event_handler = nullptr;
      g_commands.pop_back();
      DERROR("Failed to set event callbacks: 0x%08X\n", hr);
      return hr;
    }
  }
  return S_OK;
}

HRESULT CALLBACK ListBreakCommandsInternal(IDebugClient* client,
                                           const char* args) {
  if (g_commands.empty()) {
    DOUT("No break commands are currently set.\n");
  } else {
    DOUT("Current break commands:\n");
    for (size_t i = 0; i < g_commands.size(); i++) {
      DOUT("\t%u) %s\n", i, g_commands[i].c_str());
    }
    DOUT("\n");
  }
  return S_OK;
}

HRESULT CALLBACK RemoveBreakCommandsInternal(IDebugClient* client,
                                             const char* args) {
  if (args && args[0] == '?' && args[1] == '\0') {
    DOUT(
        "RemoveBreakCommands - Removes break commands\n\n"
        "Usage: !RemoveBreakCommands [index]\n\n"
        "  [index]  - Optional index of command to remove\n\n"
        "Examples:\n"
        "  !RemoveBreakCommands     - Removes all break commands\n"
        "  !RemoveBreakCommands 0   - Removes only the first break command\n"
        "  !RemoveBreakCommands 2   - Removes the break command at index 2\n\n"
        "Use !ListBreakCommands to see all existing commands with their indices\n");
    return S_OK;
  }

  std::vector<std::string> parsed_args = utils::ParseCommandLine(args);

  // Case 1: No arguments, clear all commands
  if (parsed_args.empty()) {
    size_t count = g_commands.size();
    g_commands.clear();
    DOUT("Removed all %u break commands.\n", count);
    return S_OK;
  }

  // Case 2: Clear specific command by index
  try {
    size_t index = std::stoul(parsed_args[0]);
    if (index >= g_commands.size()) {
      DERROR("Error: Index %u is out of range. Only %u commands exist.\n",
             index, g_commands.size());
      return E_INVALIDARG;
    }

    std::string removed_cmd = g_commands[index];

    g_commands.erase(g_commands.begin() + index);
    DOUT("Removed break command [%u]: %s\n", index, removed_cmd.c_str());
    return S_OK;
  } catch (const std::exception&) {
    DERROR(
        "Error: Invalid argument. Please specify either no arguments to remove all commands, "
        "or a valid index number to remove a specific command.\n");
    return E_INVALIDARG;
  }
}

// Export functions
extern "C" {
__declspec(dllexport) HRESULT CALLBACK DebugExtensionInitialize(PULONG version,
                                                                PULONG flags) {
  return DebugExtensionInitializeInternal(version, flags);
}

__declspec(dllexport) HRESULT CALLBACK DebugExtensionUninitialize(void) {
  return DebugExtensionUninitializeInternal();
}

__declspec(dllexport) HRESULT CALLBACK AddBreakCommand(IDebugClient* client,
                                                       const char* args) {
  return AddBreakCommandInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK ListBreakCommands(IDebugClient* client,
                                                         const char* args) {
  return ListBreakCommandsInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK RemoveBreakCommands(IDebugClient* client,
                                                           const char* args) {
  return RemoveBreakCommandsInternal(client, args);
}
}

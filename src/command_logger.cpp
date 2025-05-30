// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#include <dbgeng.h>
#include <windows.h>
#include <fstream>
#include <regex>
#include <string>

#include "utils.h"

utils::DebugInterfaces g_debug;

// File to log commands to
std::ofstream g_log_file;

class MyOutputCallbacks;
MyOutputCallbacks* g_output_callbacks = nullptr;

class MyOutputCallbacks : public IDebugOutputCallbacks {
 public:
  MyOutputCallbacks() : m_ref_count(1) {
    // Match common WinDbg prompts followed by any text
    m_cmd_prompt_regex = std::regex("^(\\d+:\\d+>|kd>|cdb>|lkd>|0:)\\s*(.+)");
  }

  // IUnknown methods
  STDMETHOD(QueryInterface)(REFIID InterfaceId, PVOID* Interface) {
    *Interface = NULL;
    if (IsEqualIID(InterfaceId, __uuidof(IUnknown)) ||
        IsEqualIID(InterfaceId, __uuidof(IDebugOutputCallbacks))) {
      *Interface = (IDebugOutputCallbacks*)this;
      AddRef();
      return S_OK;
    }
    return E_NOINTERFACE;
  }

  STDMETHOD_(ULONG, AddRef)() { return InterlockedIncrement(&m_ref_count); }

  STDMETHOD_(ULONG, Release)() {
    LONG ret = InterlockedDecrement(&m_ref_count);
    if (ret == 0) {
      delete this;
    }
    return ret;
  }

  // IDebugOutputCallbacks method
  STDMETHOD(Output)(ULONG mask, PCSTR text) {
    if (!g_log_file.is_open() || !text) {
      return S_OK;
    }

    // Append to our buffer
    m_buffer += text;

    // Prevent buffer from growing too large
    if (m_buffer.size() > MAX_BUFFER_SIZE) {
      m_buffer = m_buffer.substr(m_buffer.size() - MAX_BUFFER_SIZE);
    }

    // Find complete lines (ending with newlines) and process them
    size_t pos = 0;
    size_t newline;

    while ((newline = m_buffer.find('\n', pos)) != std::string::npos) {
      // Extract the complete line
      std::string line = m_buffer.substr(pos, newline - pos);

      // Remove any carriage returns
      if (!line.empty() && line.back() == '\r') {
        line.pop_back();
      }

      // Check if this line contains a prompt and command
      std::smatch matches;
      if (std::regex_match(line, matches, m_cmd_prompt_regex) &&
          matches.size() >= 3) {
        std::string prompt = matches[1].str();
        std::string command = matches[2].str();

        // Skip our extension commands
        if (command.find("!StartCommandLogging") == std::string::npos &&
            command.find("!StopCommandLogging") == std::string::npos) {
          g_log_file << command << std::endl;
          g_log_file.flush();
        }
      }

      // Move to the character after this newline
      pos = newline + 1;
    }

    // Keep only the incomplete line at the end
    if (pos > 0) {
      m_buffer = m_buffer.substr(pos);
    }

    return S_OK;
  }

 private:
  LONG m_ref_count;

  std::string m_buffer;
  static constexpr size_t MAX_BUFFER_SIZE = 16384;

  std::regex m_cmd_prompt_regex;
};

HRESULT CALLBACK DebugExtensionInitializeInternal(PULONG version,
                                                  PULONG flags) {
  *version = DEBUG_EXTENSION_VERSION(1, 0);
  *flags = 0;

  return utils::InitializeDebugInterfaces(&g_debug);
}

HRESULT CALLBACK DebugExtensionUninitializeInternal() {
  if (g_log_file.is_open()) {
    g_log_file.close();
  }

  // Remove the output callbacks if active
  if (g_output_callbacks) {
    g_debug.client->SetOutputCallbacks(nullptr);
    g_output_callbacks->Release();
    g_output_callbacks = nullptr;
  }

  return utils::UninitializeDebugInterfaces(&g_debug);
}

HRESULT CALLBACK StartCommandLoggingInternal(IDebugClient* client,
                                             const char* args) {
  if (args && args[0] == '?' && args[1] == '\0') {
    DOUT(
        "StartCommandLogging - Begin logging WinDbg commands to a file\n\n"
        "Usage: !StartCommandLogging <filename>\n\n"
        "  <filename> - Path to the log file (required)\n\n"
        "Examples:\n"
        "  !StartCommandLogging c:\\temp\\windbg_commands.log\n\n"
        "Commands are appended to the file if it already exists.\n"
        "Use !StopCommandLogging to end the logging session.\n\n");
    return S_OK;
  }

  if (g_log_file.is_open()) {
    DERROR("Command logging is already active.\n");
    return S_OK;
  }

  // Determine the log filename
  std::string log_file_path;

  if (args && *args) {
    // Parse the arguments to get the filename and trim it
    log_file_path = utils::Trim(args);
  }

  if (log_file_path.empty()) {
    DERROR("Invalid filename provided.\n");
    return E_FAIL;
  }

  g_log_file.open(log_file_path, std::ios::app);
  if (!g_log_file.is_open()) {
    DERROR("Failed to open log file: %s\n", log_file_path.c_str());
    return E_FAIL;
  }

  // Create and register the output callbacks
  if (!g_output_callbacks) {
    g_output_callbacks = new MyOutputCallbacks();
    g_debug.client->SetOutputCallbacks(g_output_callbacks);
  }

  DOUT("Command logging started. Commands will be saved to %s\n",
       log_file_path.c_str());

  return S_OK;
}

HRESULT CALLBACK StopCommandLoggingInternal(IDebugClient* client,
                                            const char* args) {
  // Check for help request
  if (args && args[0] == '?' && args[1] == '\0') {
    DOUT(
        "StopCommandLogging - End the current command logging session\n\n"
        "Usage: !StopCommandLogging\n\n"
        "This command takes no parameters.\n"
        "Stops the active logging session started with !StartCommandLogging.\n\n");
    return S_OK;
  }

  if (!g_log_file.is_open()) {
    DERROR("Command logging is not active.\n");
    return S_OK;
  } else {
    g_log_file.close();
  }

  // Remove the output callbacks
  if (g_output_callbacks) {
    g_debug.client->SetOutputCallbacks(nullptr);
    g_output_callbacks->Release();
    g_output_callbacks = nullptr;
  }

  g_debug.control->Output(DEBUG_OUTPUT_NORMAL, "Command logging stopped.\n");
  return S_OK;
}

// Required export functions
extern "C" {
__declspec(dllexport) HRESULT CALLBACK DebugExtensionInitialize(PULONG version,
                                                                PULONG flags) {
  return DebugExtensionInitializeInternal(version, flags);
}

__declspec(dllexport) HRESULT CALLBACK DebugExtensionUninitialize(void) {
  return DebugExtensionUninitializeInternal();
}

__declspec(dllexport) HRESULT CALLBACK StartCommandLogging(IDebugClient* client,
                                                           const char* args) {
  return StartCommandLoggingInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK StopCommandLogging(IDebugClient* client,
                                                          const char* args) {
  return StopCommandLoggingInternal(client, args);
}
}

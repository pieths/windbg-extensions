// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#include <dbgeng.h>
#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>

#include "utils.h"

utils::DebugInterfaces g_debug;

// Forward declarations
std::string GetProcessCommandLine();
std::string ClassifyBrowserProcessType(const std::string& command_line);

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

  std::string command_line = GetProcessCommandLine();

  if (command_line.empty()) {
      DOUT("Error: Failed to get command line information.\n");
      DOUT("This could indicate that no process is being debugged, the debugger is not attached properly,\n");
      DOUT("or the PEB format is different than expected.\n");
      return E_FAIL;
  }

  DOUT("%s\n", command_line.c_str());
  return S_OK;
}

std::string GetProcessCommandLine() {
  try {
    // Get the Process Environment Block (PEB) to access command line
    std::string peb_command = "!peb";
    std::string peb_output = utils::ExecuteCommand(&g_debug, peb_command);
    
    if (peb_output.empty()) {
      return "";
    }
    
    // Look for CommandLine in the PEB output
    size_t cmd_line_pos = peb_output.find("CommandLine:");
    if (cmd_line_pos == std::string::npos) {
      return "";
    }
    
    // Extract the command line portion
    size_t start_pos = peb_output.find("'", cmd_line_pos);
    if (start_pos == std::string::npos) {
      return "";
    }
    start_pos++; // Skip the opening quote
    
    size_t end_pos = peb_output.find("'", start_pos);
    if (end_pos == std::string::npos) {
      return "";
    }
    
    return peb_output.substr(start_pos, end_pos - start_pos);
    
  } catch (...) {
    return "";
  }
}

std::string ClassifyBrowserProcessType(const std::string& command_line) {
  if (command_line.empty()) {
    return "Unknown";
  }
  
  // Convert to lowercase for case-insensitive matching
  std::string cmd_lower = command_line;
  std::transform(cmd_lower.begin(), cmd_lower.end(), cmd_lower.begin(), ::tolower);
  
  // Check for specific process types based on command line arguments
  if (cmd_lower.find("--type=renderer") != std::string::npos) {
    return "Renderer";
  }
  else if (cmd_lower.find("--type=gpu-process") != std::string::npos) {
    return "GPU";
  }
  else if (cmd_lower.find("--type=utility") != std::string::npos) {
    // Check for specific utility process subtypes
    if (cmd_lower.find("--utility-sub-type=network.mojom.networkservice") != std::string::npos ||
        cmd_lower.find("network.mojom.networkservice") != std::string::npos) {
      return "Utility:Network";
    }
    else if (cmd_lower.find("--utility-sub-type=audio.mojom.audioservice") != std::string::npos ||
             cmd_lower.find("audio.mojom.audioservice") != std::string::npos) {
      return "Utility:Audio";
    }
    else if (cmd_lower.find("--utility-sub-type=storage.mojom.storageservice") != std::string::npos ||
             cmd_lower.find("storage.mojom.storageservice") != std::string::npos) {
      return "Utility:Storage";
    }
    else if (cmd_lower.find("--utility-sub-type=video_capture.mojom.videocaptureservice") != std::string::npos ||
             cmd_lower.find("video_capture.mojom.videocaptureservice") != std::string::npos) {
      return "Utility:VideoCapture";
    }
    else {
      // Try to extract the utility sub-type
      size_t sub_type_pos = cmd_lower.find("--utility-sub-type=");
      if (sub_type_pos != std::string::npos) {
        size_t sub_type_start = sub_type_pos + 19; // Length of "--utility-sub-type="
        size_t sub_type_end = cmd_lower.find(" ", sub_type_start);
        if (sub_type_end == std::string::npos) {
          sub_type_end = cmd_lower.length();
        }
        std::string sub_type = cmd_lower.substr(sub_type_start, sub_type_end - sub_type_start);
        return "Utility:" + sub_type;
      }
      return "Utility:Other";
    }
  }
  else if (cmd_lower.find("--type=ppapi") != std::string::npos) {
    return "PPAPI";
  }
  else if (cmd_lower.find("--type=crashpad-handler") != std::string::npos) {
    return "CrashHandler";
  }
  else if (cmd_lower.find("--type=zygote") != std::string::npos) {
    return "Zygote";
  }
  else if (cmd_lower.find("--type=sandbox-helper") != std::string::npos) {
    return "SandboxHelper";
  }
  else if (cmd_lower.find("--type=") != std::string::npos) {
    // Extract the process type if it's something we don't recognize
    size_t type_pos = cmd_lower.find("--type=");
    size_t type_start = type_pos + 7; // Length of "--type="
    size_t type_end = cmd_lower.find(" ", type_start);
    if (type_end == std::string::npos) {
      type_end = cmd_lower.length();
    }
    std::string process_type = cmd_lower.substr(type_start, type_end - type_start);
    return "Unknown:" + process_type;
  }
  else {
    // No --type argument, likely the main browser process
    return "Browser";
  }
}

HRESULT CALLBACK BrowserProcessTypeInternal(IDebugClient* client, const char* args) {
  if (args && *args && (args[0] == '?' && args[1] == '\0')) {
    DOUT(
        "BrowserProcessType - Classifies the type of browser process.\n\n"
        "Usage: !BrowserProcessType\n"
        "Alias: !#bpt\n\n"
        "This command analyzes the command line of the current process\n"
        "and classifies it as one of the following browser process types:\n\n"
        "  Browser          - Main browser process\n"
        "  Renderer         - Web content renderer process\n"
        "  GPU              - GPU process\n"
        "  Utility:Network  - Network service utility process\n"
        "  Utility:Audio    - Audio service utility process\n"
        "  Utility:Storage  - Storage service utility process\n"
        "  Utility:VideoCapture - Video capture utility process\n"
        "  Utility:Other    - Other utility process\n"
        "  PPAPI            - PPAPI plugin process\n"
        "  CrashHandler     - Crashpad handler process\n"
        "  Zygote           - Zygote process (Linux)\n"
        "  SandboxHelper    - Sandbox helper process\n"
        "  Unknown:<type>   - Unrecognized process type\n"
        "  Unknown          - Could not determine process type\n");
    return S_OK;
  }

  try {
    std::string command_line = GetProcessCommandLine();
    
    if (command_line.empty()) {
      DOUT("Error: Could not retrieve command line for process classification.\n");
      return E_FAIL;
    }
    
    std::string process_type = ClassifyBrowserProcessType(command_line);
    
    DOUT("%s\n", process_type.c_str());
    
  } catch (const std::exception& e) {
    DOUT("Error classifying browser process type: %s\n", e.what());
    return E_FAIL;
  } catch (...) {
    DOUT("Unknown error classifying browser process type.\n");
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

__declspec(dllexport) HRESULT CALLBACK BrowserProcessType(IDebugClient* client, const char* args) {
  return BrowserProcessTypeInternal(client, args);
}

}  // extern "C"

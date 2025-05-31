// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#include "utils.h"

#include <algorithm>

namespace utils {

namespace {

class CommandOutputCapture : public IDebugOutputCallbacks {
 public:
  CommandOutputCapture() : m_ref_count(1), m_capturing(false) {}

  // IUnknown methods
  STDMETHOD(QueryInterface)(REFIID InterfaceId, PVOID* Interface) {
    *Interface = nullptr;
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
    if (m_capturing && text) {
      m_output += text;
    }
    return S_OK;
  }

  void StartCapture() {
    m_capturing = true;
    m_output.clear();
  }

  void StopCapture() { m_capturing = false; }

  std::string GetOutput() const { return m_output; }

 private:
  LONG m_ref_count;
  bool m_capturing;
  std::string m_output;
};

// Wait for the debugger status to indicate a break.
// This method will time out after 20 seconds.
// If an error occurs while waiting or retrieving the status
// then the method will return early.
void WaitForBreakStatus(const DebugInterfaces* interfaces) {
  HRESULT hr;
  ULONG status;
  int retries = 0;

  while (true) {
    hr = interfaces->control->GetExecutionStatus(&status);
    if (FAILED(hr) && retries < 10) {
      Sleep(100);
    } else {
      break;
    }
    retries++;
  }

  if (FAILED(hr)) {
    return;
  }

  if (status == DEBUG_STATUS_GO || status == DEBUG_STATUS_GO_HANDLED ||
      status == DEBUG_STATUS_GO_NOT_HANDLED ||
      status == DEBUG_STATUS_STEP_INTO || status == DEBUG_STATUS_STEP_OVER ||
      status == DEBUG_STATUS_STEP_BRANCH) {
    // Wait for a total of 30 seconds for the target to break
    hr = interfaces->control->WaitForEvent(DEBUG_WAIT_DEFAULT, 20000);
    if (FAILED(hr)) {
      return;
    }
  }
}

}  // namespace

std::string Trim(const std::string& str) {
  size_t start = str.find_first_not_of(" \t\n\r\f\v");
  size_t end = str.find_last_not_of(" \t\n\r\f\v");
  return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

bool ContainsCI(const std::string& string, const std::string& substr) {
  auto it = std::search(
      string.begin(), string.end(), substr.begin(), substr.end(),
      [](char a, char b) { return std::tolower(a) == std::tolower(b); });
  return it != string.end();
}

bool IsWholeNumber(const std::string& input_str) {
  if (input_str.empty()) {
    return false;
  }
  return std::all_of(input_str.begin(), input_str.end(), ::isdigit);
}

std::string RemoveFileExtension(const std::string& filename) {
  // Find the last dot in the filename
  size_t last_dot = filename.find_last_of(".");

  // If no dot is found or it's at the beginning of the string
  // (hidden file in Unix), return the original string
  if (last_dot == std::string::npos || last_dot == 0) {
    return filename;
  }

  // Check if there's a directory separator after the last dot
  // If so, this is not an extension but part of a directory name
  size_t last_separator = filename.find_last_of("/\\");
  if (last_separator != std::string::npos && last_separator > last_dot) {
    return filename;
  }

  // Return the substring before the last dot
  return filename.substr(0, last_dot);
}

std::string EscapeQuotes(const std::string& input) {
  std::string escaped;
  size_t backslash_count = 0;

  for (size_t i = 0; i < input.length(); ++i) {
    if (input[i] == '\\') {
      // Count consecutive backslashes
      backslash_count++;
      escaped += '\\';
    } else if (input[i] == '"') {
      // If even number of backslashes (including 0), the quote is not escaped
      if (backslash_count % 2 == 0) {
        escaped += "\\\"";
      } else {
        // Odd number of backslashes means the quote is already escaped
        escaped += '"';
      }
      backslash_count = 0;  // Reset after encountering a quote
    } else {
      // Any other character resets the backslash count
      backslash_count = 0;
      escaped += input[i];
    }
  }
  return escaped;
}

std::string GetCurrentExtensionDir() {
  HMODULE hModule = NULL;

  // Get the handle to the current DLL
  BOOL result = GetModuleHandleEx(
      GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
          GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
      (LPCSTR)&GetCurrentExtensionDir,  // Use the address of this function as a
                                        // reference point
      &hModule);

  if (!result || hModule == NULL) {
    // Fallback if we couldn't get the module handle
    return "";
  }

  char buffer[MAX_PATH] = {0};
  GetModuleFileNameA(hModule, buffer, MAX_PATH);
  std::string path(buffer);
  size_t last_slash = path.find_last_of("\\");
  if (last_slash != std::string::npos) {
    return path.substr(0, last_slash);
  }
  return "";
}

std::vector<std::string> SplitString(const std::string& input,
                                     const std::string& delimiter,
                                     bool combine_consecutive_delimiters) {
  std::vector<std::string> tokens;
  if (input.empty()) {
    return tokens;
  }

  size_t start = 0;
  size_t end = input.find(delimiter);

  while (end != std::string::npos) {
    if (!combine_consecutive_delimiters || end > start) {
      tokens.push_back(input.substr(start, end - start));
    }
    start = end + delimiter.length();
    end = input.find(delimiter, start);
  }

  // Add the last token
  if (start < input.length()) {
    tokens.push_back(input.substr(start));
  }

  return tokens;
}

std::vector<size_t> GetIndicesFromString(const std::string& input_str) {
  std::vector<size_t> indices;
  std::vector<std::string> tokens;

  // First split by spaces
  std::vector<std::string> space_tokens =
      utils::SplitString(utils::Trim(input_str), " ");

  for (const auto& token : space_tokens) {
    // Check if token contains a range (has a hyphen)
    if (token.find('-') != std::string::npos) {
      std::vector<std::string> range_parts = utils::SplitString(token, "-");
      if (range_parts.size() == 2) {
        int start = std::stoi(range_parts[0]);
        int end = std::stoi(range_parts[1]);

        // Handle both ascending and descending ranges
        if (start <= end) {
          for (int i = start; i <= end; i++) {
            indices.push_back(i);
          }
        } else {
          for (int i = start; i >= end; i--) {
            indices.push_back(i);
          }
        }
      } else {
        // Invalid range format
        continue;
      }
    } else {
      // Regular number
      indices.push_back(std::stoi(token));
    }
  }

  // Sort and remove duplicates
  std::sort(indices.begin(), indices.end());
  indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
  return indices;
}

bool ParseNumberOrDottedPair(const std::string& input,
                             size_t& first_number,
                             std::optional<size_t>& second_number) {
  second_number.reset();

  if (input.empty()) {
    return false;
  }

  size_t dot_pos = input.find('.');
  bool has_dot = (dot_pos != std::string::npos);

  if (has_dot) {
    // Format: "number1.number2"
    std::string first_part = input.substr(0, dot_pos);
    std::string second_part = input.substr(dot_pos + 1);

    if (IsWholeNumber(first_part) && IsWholeNumber(second_part)) {
      first_number = std::stoul(first_part);
      second_number = std::stoul(second_part);
      return true;
    }
  } else {
    // Single number format
    if (IsWholeNumber(input)) {
      first_number = std::stoul(input);
      return true;
    }
  }

  return false;
}

std::vector<std::string> ParseCommandLine(const char* cmd_line) {
  std::vector<std::string> args;
  if (!cmd_line || !*cmd_line) {
    return args;
  }

  bool in_single_quotes = false;
  bool is_escaped = false;
  std::string current_arg;

  for (const char* p = cmd_line; *p; p++) {
    if (is_escaped) {
      // Add escaped character
      current_arg += *p;
      is_escaped = false;
      continue;
    }

    if (*p == '\\') {
      is_escaped = true;
      continue;
    }

    if (*p == '\'') {
      if (!current_arg.empty() || in_single_quotes) {
        args.push_back(current_arg);
        current_arg.clear();
      }
      in_single_quotes = !in_single_quotes;
      continue;
    }

    if (isspace(*p) && !in_single_quotes) {
      // End of argument
      if (!current_arg.empty()) {
        args.push_back(current_arg);
        current_arg.clear();
      }
      continue;
    }

    // Regular character
    current_arg += *p;
  }

  // Add the last argument if exists
  if (!current_arg.empty()) {
    args.push_back(current_arg);
  }

  return args;
}

SourceInfo GetCurrentSourceInfo(const DebugInterfaces* interfaces) {
  SourceInfo info;

  if (!interfaces || !interfaces->control || !interfaces->symbols) {
    return info;
  }

  // Get the current instruction pointer
  ULONG64 offset = 0;
  HRESULT hr = interfaces->symbols->GetScope(&offset, nullptr, nullptr, 0);
  if (FAILED(hr)) {
    return info;
  }

  // Get source file and line information for the current position
  char file_path[MAX_PATH] = {0};
  ULONG line_number = 0;

  hr = interfaces->symbols->GetLineByOffset(
      offset, &line_number, file_path, sizeof(file_path), nullptr, nullptr);
  if (SUCCEEDED(hr) && file_path[0] != '\0') {
    info.line = line_number;
    std::string full_path = std::string(file_path);

    // Separate file name from path
    size_t last_slash = full_path.find_last_of("\\/");
    if (last_slash != std::string::npos) {
      info.file_name = full_path.substr(last_slash + 1);
      info.file_path =
          full_path.substr(0, last_slash);  // Store only the path portion
    } else {
      // No slashes found, the whole string is the filename
      info.file_name = full_path;
      info.file_path = "";  // No path component
    }

    info.full_path = full_path;
    info.source_context = ExecuteCommand(interfaces, "lsa .");
    info.is_valid = true;
  }

  return info;
}

HRESULT InitializeDebugInterfaces(DebugInterfaces* interfaces) {
  if (!interfaces) {
    return E_INVALIDARG;
  }

  interfaces->client = nullptr;
  interfaces->control = nullptr;
  interfaces->symbols = nullptr;

  HRESULT hr = DebugCreate(__uuidof(IDebugClient), (void**)&interfaces->client);
  if (FAILED(hr)) {
    return hr;
  }

  hr = interfaces->client->QueryInterface(__uuidof(IDebugControl),
                                          (void**)&interfaces->control);
  if (FAILED(hr)) {
    interfaces->client->Release();
    interfaces->client = nullptr;
    return hr;
  }

  hr = interfaces->client->QueryInterface(__uuidof(IDebugSymbols),
                                          (void**)&interfaces->symbols);
  if (FAILED(hr)) {
    interfaces->control->Release();
    interfaces->client->Release();
    interfaces->control = nullptr;
    interfaces->client = nullptr;
    return hr;
  }

  return S_OK;
}

HRESULT UninitializeDebugInterfaces(DebugInterfaces* interfaces) {
  if (!interfaces) {
    return E_INVALIDARG;
  }

  // Release interfaces in reverse order of acquisition
  if (interfaces->symbols) {
    interfaces->symbols->Release();
    interfaces->symbols = nullptr;
  }

  if (interfaces->control) {
    interfaces->control->Release();
    interfaces->control = nullptr;
  }

  if (interfaces->client) {
    interfaces->client->Release();
    interfaces->client = nullptr;
  }

  return S_OK;
}

std::string ExecuteCommand(const DebugInterfaces* interfaces,
                           const std::string& command,
                           bool wait_for_break_status) {
  if (!interfaces || !interfaces->client || !interfaces->control) {
    return "";
  }

  if (wait_for_break_status) {
    WaitForBreakStatus(interfaces);
  }

  // Save the current output callbacks
  IDebugOutputCallbacks* previous_callbacks = nullptr;
  interfaces->client->GetOutputCallbacks(&previous_callbacks);

  // Create and set our capture callbacks
  CommandOutputCapture* capture = new CommandOutputCapture();
  interfaces->client->SetOutputCallbacks(capture);

  // TODO: "x ..." commands are not working correctly
  // Execute the command and capture output
  capture->StartCapture();
  HRESULT hr = interfaces->control->Execute(
      DEBUG_OUTCTL_THIS_CLIENT, command.c_str(), DEBUG_EXECUTE_DEFAULT);

  if (wait_for_break_status) {
    WaitForBreakStatus(interfaces);
  }

  capture->StopCapture();

  std::string output = capture->GetOutput();

  // Restore previous callbacks
  interfaces->client->SetOutputCallbacks(previous_callbacks);
  if (previous_callbacks) {
    previous_callbacks->Release();
  }

  // Release our capture object
  capture->Release();

  return SUCCEEDED(hr) ? output : "";
}

}  // namespace utils

// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#include <dbgeng.h>
#include <windows.h>
#include <algorithm>
#include <fstream>
#include <regex>
#include <set>
#include <string>
#include <vector>

#include "breakpoint_list.h"
#include "debug_event_callbacks.h"
#include "json.hpp"
#include "utils.h"

using JSON = nlohmann::json;

// WinDbg extension globals
utils::DebugInterfaces g_debug;

// Global variables to store breakpoint history
std::vector<BreakpointList> g_breakpoint_lists;
std::string g_breakpoint_lists_file;
BreakpointList g_breakpoint_list;

class EventCallbacks : public DebugEventCallbacks {
 public:
  EventCallbacks() : DebugEventCallbacks(DEBUG_EVENT_LOAD_MODULE) {}

  STDMETHOD(LoadModule)(ULONG64 ImageFileHandle,
                        ULONG64 BaseOffset,
                        ULONG ModuleSize,
                        PCSTR ModuleName,
                        PCSTR ImageName,
                        ULONG CheckSum,
                        ULONG TimeDateStamp) {
    // ModuleName: "chrome"
    // ImageName: "D:\cs\src\out\release_x64\chrome.dll"
    std::string module_name(ModuleName);

    // TODO: correctly handle BreakpointLists which might
    // have module names that contain .dll or .exe extensions.

    if (!g_breakpoint_list.IsValid()) {
      // If no global breakpoints are set,
      // we don't need to do anything
      return DEBUG_STATUS_NO_CHANGE;
    }

    // TODO: update the extension handling so that
    // we handle both .exe and .dll files?

    bool found_breakpoints = false;

    // Find all breakpoints that match this module
    const auto& breakpoints = g_breakpoint_list.GetBreakpoints();
    for (const auto& bp : breakpoints) {
      if (bp.GetModuleName() == module_name) {
        if (!found_breakpoints) {
          DOUT("\nModule loaded: [%s] - Setting breakpoints...\n",
               module_name.c_str());
          found_breakpoints = true;
        }

        std::string bp_command = "bp " + bp.GetFullString();
        DOUT("    %s\n", bp_command.c_str());

        g_debug.control->Execute(DEBUG_OUTCTL_ALL_CLIENTS, bp_command.c_str(),
                                 DEBUG_EXECUTE_DEFAULT);
      }
    }

    if (found_breakpoints) {
      DOUT("\n");
    }

    return DEBUG_STATUS_NO_CHANGE;
  }
};

EventCallbacks* g_event_callbacks = nullptr;

void InitializeBreakpoints() {
  if (g_breakpoint_lists_file.empty()) {
    g_breakpoint_lists_file =
        // Use the same location as the DLL if no path was set
        utils::GetCurrentExtensionDir() + "\\breakpoints_history.json";
  }

  std::ifstream file(g_breakpoint_lists_file);
  if (file.is_open()) {
    try {
      JSON json;
      file >> json;
      file.close();

      g_breakpoint_lists.clear();
      if (json.is_array()) {
        for (const auto& item : json) {
          g_breakpoint_lists.push_back(BreakpointList::FromJson(item));
        }
      }
    } catch (const std::exception& e) {
      DERROR("Error loading breakpoint history: %s\n", e.what());
      g_breakpoint_lists.clear();
    }
  } else {
    g_breakpoint_lists.clear();
  }
}

void WriteBreakpointsToFile() {
  if (g_breakpoint_lists.empty()) {
    DOUT("No breakpoints to save.\n");
    return;
  }

  try {
    JSON json = JSON::array();
    for (const auto& bp_list : g_breakpoint_lists) {
      json.push_back(bp_list.ToJson());
    }

    std::ofstream file(g_breakpoint_lists_file);
    if (file.is_open()) {
      file << json.dump(4);
      file.close();
    } else {
      DERROR("Error: Cannot open file for writing: %s\n",
             g_breakpoint_lists_file.c_str());
    }
  } catch (const std::exception& e) {
    DERROR("Error saving breakpoint history: %s\n", e.what());
  }
}

// Filter breakpoint lists based on search term
std::vector<std::pair<size_t, BreakpointList>> GetFilteredBreakpointLists(
    const std::string& search_term,
    size_t count = 0) {
  std::vector<std::pair<size_t, BreakpointList>> filtered_list;

  bool ignore_search_term = search_term.empty();

  for (size_t i = 0; i < g_breakpoint_lists.size(); i++) {
    if (ignore_search_term || g_breakpoint_lists[i].HasTextMatch(search_term)) {
      filtered_list.emplace_back(i, g_breakpoint_lists[i]);
    }
  }

  if (count > 0 && filtered_list.size() > count) {
    filtered_list.resize(count);
  }
  return filtered_list;
}

// Helper method to parse a space-separated list of numbers with optional
// .number suffix and return a breakpoint list combining selected breakpoints
// Format: "1 2 3.0 4.1" where numbers are indices into g_breakpoint_lists
// and optional .n suffix is the index into that breakpoint list's breakpoints
BreakpointList GetBreakpointListFromNumberString(
    const std::string& number_string) {
  std::vector<std::string> indices =
      utils::SplitString(utils::Trim(number_string), " ");
  BreakpointList breakpoint_list;

  for (const auto& index_str : indices) {
    // Split by dot to get list index and optional breakpoint index
    std::vector<std::string> parts = utils::SplitString(index_str, ".");

    if (parts.empty()) {
      continue;
    }

    // Parse the list index
    int list_index;
    try {
      list_index = std::stoi(parts[0]);
    } catch (const std::exception&) {
      DERROR("Invalid breakpoint list index: %s\n", parts[0].c_str());
      continue;
    }

    // Validate list index range
    if (list_index < 0 ||
        list_index >= static_cast<int>(g_breakpoint_lists.size())) {
      DERROR("Invalid breakpoint list index: %d (out of range)\n", list_index);
      continue;
    }

    // Get the breakpoint list at the specified index
    const BreakpointList& current_list = g_breakpoint_lists[list_index];

    if (parts.size() == 1) {
      // Use all breakpoints from this list
      for (const auto& bp : current_list.GetBreakpoints()) {
        breakpoint_list.AddBreakpoint(bp);
      }
    } else {
      // Use specific breakpoint at the given index
      int bp_index;
      try {
        bp_index = std::stoi(parts[1]);
      } catch (const std::exception&) {
        DERROR("Invalid breakpoint index: %s\n", parts[1].c_str());
        continue;
      }

      // Get the specific breakpoint
      Breakpoint bp = current_list.GetBreakpointAtIndex(bp_index);
      if (!bp.IsValid()) {
        DERROR("Invalid breakpoint index %d for list at index %d\n", bp_index,
               list_index);
        continue;
      } else {
        breakpoint_list.AddBreakpoint(bp);
      }
    }
  }

  if (breakpoint_list.GetBreakpointsCount() == 0) {
    DERROR("No valid breakpoints found from the specified indices.\n");
  } else {
    // Get tag from the first list if available
    if (!g_breakpoint_lists.empty() && !indices.empty()) {
      int first_index = std::stoi(utils::SplitString(indices[0], ".")[0]);
      if (first_index >= 0 &&
          first_index < static_cast<int>(g_breakpoint_lists.size())) {
        breakpoint_list.SetTag(g_breakpoint_lists[first_index].GetTag());
      }
    }
  }

  return breakpoint_list;
}

BreakpointList GetBreakpointListFromSearchTerm(const std::string& search_term) {
  BreakpointList breakpoint_list;

  if (search_term.empty()) {
    DERROR("No search term provided.\n");
    return breakpoint_list;
  }

  auto filtered_lists = GetFilteredBreakpointLists(search_term);
  if (filtered_lists.empty()) {
    DERROR("\nNo breakpoints found matching: %s\n\n", search_term.c_str());
    return breakpoint_list;
  }

  return filtered_lists[0].second;
}

BreakpointList GetBreakpointListFromTagMatch(const std::string& tag) {
  BreakpointList breakpoint_list;

  if (tag.empty()) {
    DERROR("No tag provided.\n");
    return breakpoint_list;
  }

  std::vector<BreakpointList> tagged_lists;
  for (const auto& bl : g_breakpoint_lists) {
    if (bl.HasTagMatch(tag)) {
      tagged_lists.push_back(bl);
    }
  }

  if (tagged_lists.empty()) {
    DERROR("\nNo breakpoints found matching tag: %s\n\n", tag.c_str());
    return breakpoint_list;
  } else if (tagged_lists.size() == 1) {
    breakpoint_list = tagged_lists[0];
  } else {
    // Combine all matching breakpoint lists
    breakpoint_list = tagged_lists[0];
    for (size_t i = 1; i < tagged_lists.size(); i++) {
      breakpoint_list = BreakpointList::CombineBreakpointLists(breakpoint_list,
                                                               tagged_lists[i]);
    }
  }

  return breakpoint_list;
}

BreakpointList GetBreakpointListFromCombinedFormat(
    const std::string& input,
    const std::string& new_module_name) {
  BreakpointList breakpoint_list;

  std::vector<std::string> parts = utils::SplitString(input, "+", false);
  if (parts.size() != 2) {
    DERROR(
        "Invalid format for combined breakpoints. Expected: '<numbers> + <breakpoints>'\n");
    return breakpoint_list;
  }

  std::string numbers_part = utils::Trim(parts[0]);
  std::string new_bp_part = utils::Trim(parts[1]);

  if (numbers_part.empty()) {
    DERROR("No breakpoint indices provided before '+'\n");
    return breakpoint_list;
  }

  if (new_bp_part.empty()) {
    DERROR("No new breakpoints provided after '+'\n");
    return breakpoint_list;
  }

  // First get breakpoints from history
  BreakpointList new_bp_list1 = GetBreakpointListFromNumberString(numbers_part);
  if (!new_bp_list1.IsValid()) {
    DERROR("Failed to get valid breakpoints from specified indices\n");
    return breakpoint_list;
  }

  // Then combine with new breakpoints
  std::string module_name = new_module_name;
  if (module_name.empty()) {
    // Get module name from the first breakpoint if available
    if (new_bp_list1.GetBreakpointsCount() > 0) {
      module_name = new_bp_list1.GetBreakpointAtIndex(0).GetModuleName();
    }
    if (module_name.empty()) {
      module_name = "chrome.dll";
      DERROR("No module name provided. Using the default: \"chrome.dll\"\n");
    }
  }

  BreakpointList new_bp_list2(new_bp_part, module_name, "");
  if (!new_bp_list2.IsValid()) {
    DERROR("Failed to create new breakpoints from: %s\n", new_bp_part.c_str());
    return breakpoint_list;
  }

  // Combine the two lists
  breakpoint_list =
      BreakpointList::CombineBreakpointLists(new_bp_list1, new_bp_list2);
  return breakpoint_list;
}

BreakpointList GetBreakpointListFromCurrentLocationOrLine(
    const std::string& input,
    const std::string& new_module_name,
    const std::string& new_tag) {
  BreakpointList breakpoint_list;
  int absolute_line = 0;

  // Extract line number if specified (format is ".:123")
  if (input.size() > 1) {
    try {
      absolute_line = std::stoi(input.substr(2));
      if (absolute_line < 1) {
        DERROR("Invalid line number: %d. Line numbers must be positive.\n",
               absolute_line);
        return breakpoint_list;
      }
    } catch (const std::exception&) {
      DERROR("Invalid line number format. Expected '.:number'\n");
      return breakpoint_list;
    }
  }

  // Get current instruction pointer
  ULONG64 offset = 0;
  PDEBUG_STACK_FRAME frame = nullptr;
  HRESULT hr = g_debug.symbols->GetScope(&offset, frame, nullptr, 0);
  if (FAILED(hr)) {
    DERROR("Failed to get current scope (error: 0x%08X)\n", hr);
    return breakpoint_list;
  }

  // Get module name, file name and line number for current position
  char file_name[MAX_PATH] = {0};
  ULONG line_number = 0;
  hr = g_debug.symbols->GetLineByOffset(offset, &line_number, file_name,
                                        sizeof(file_name), nullptr, nullptr);

  // Get module name
  std::string module_name;
  if (!new_module_name.empty()) {
    module_name = new_module_name;
  } else {
    ULONG64 base = 0;
    hr = g_debug.symbols->GetModuleByOffset(offset, 0, nullptr, &base);
    if (SUCCEEDED(hr)) {
      char module_name_buffer[MAX_PATH] = {0};
      // GetModuleNames is the correct method - use the ModuleName part
      if (SUCCEEDED(g_debug.symbols->GetModuleNames(
              DEBUG_ANY_ID, base, nullptr, 0, nullptr, nullptr, 0, nullptr,
              module_name_buffer, sizeof(module_name_buffer), nullptr))) {
        module_name = module_name_buffer;

        // Extract the module name from the full path
        size_t last_slash = module_name.find_last_of("\\");
        if (last_slash != std::string::npos) {
          module_name = module_name.substr(last_slash + 1);
        }
      }
    }
  }

  if (SUCCEEDED(hr) && file_name[0] != '\0') {
    // If this is ".", use current line, otherwise use the specified absolute
    // line
    if (absolute_line > 0) {
      line_number = absolute_line;
    }

    // Create source line breakpoint
    std::string bp_str = "`";
    if (!module_name.empty()) {
      bp_str += utils::RemoveFileExtension(module_name) + "!";
    }
    bp_str += std::string(file_name) + ":" + std::to_string(line_number) + "`";

    // Replace all backslashes with double backslashes
    bp_str = std::regex_replace(bp_str, std::regex("\\\\"), "\\\\");

    breakpoint_list = BreakpointList(bp_str, module_name, new_tag);
  } else {
    DERROR("Failed to get file name or line number (error: 0x%08X)\n", hr);
  }

  return breakpoint_list;
}

// Helper to extract a BreakpointList from different input formats
BreakpointList GetBreakpointListFromArgs(const std::string input_str,
                                         const std::string new_module_name,
                                         const std::string new_tag) {
  BreakpointList breakpoint_list;
  bool skip_tag_update = false;
  bool replace_all_modules = false;
  std::string module_name = new_module_name;

  // Check if module name starts with "+" indicating replace all mode
  if (!module_name.empty() && module_name[0] == '+') {
    replace_all_modules = true;
    module_name = module_name.substr(1);  // Remove the "+" prefix
  }

  if (input_str.empty()) {
    // If no breakpoints are provided, use the first one in history
    if (g_breakpoint_lists.empty()) {
      DERROR("No breakpoint history available.\n");
      return breakpoint_list;
    }

    breakpoint_list = g_breakpoint_lists[0];

  } else if (utils::IsWholeNumber(input_str)) {
    // Handle single index
    int index = std::stoi(input_str);
    if (index < 0 || index >= static_cast<int>(g_breakpoint_lists.size())) {
      DERROR("Invalid index: %d\n", index);
      return breakpoint_list;
    }

    breakpoint_list = g_breakpoint_lists[index];

  } else if (std::regex_match(
                 input_str,
                 std::regex("\\s*\\d+(\\.\\d+)?(\\s+\\d+(\\.\\d+)?)*\\s*"))) {
    // Handle space-separated list of numbers with optional .number suffix
    breakpoint_list = GetBreakpointListFromNumberString(input_str);

  } else if (input_str.substr(0, 2) == "s:") {
    // Filter breakpoints by search term
    std::string search_term = utils::Trim(input_str.substr(2));
    breakpoint_list = GetBreakpointListFromSearchTerm(search_term);

  } else if (input_str.substr(0, 2) == "t:") {
    // Search by tag
    std::string tag = utils::Trim(input_str.substr(2));
    breakpoint_list = GetBreakpointListFromTagMatch(tag);

  } else if (input_str.find("+") != std::string::npos) {
    // Handle combined format: "<list of numbers> + <new breakpoints>"
    breakpoint_list =
        GetBreakpointListFromCombinedFormat(input_str, module_name);

  } else if (input_str == "." ||
             std::regex_match(input_str, std::regex("\\.:\\d+"))) {
    // Handle "." for current location or ".:line_number" for absolute line in
    // current file
    breakpoint_list = GetBreakpointListFromCurrentLocationOrLine(
        input_str, module_name, new_tag);
    skip_tag_update = true;

  } else {
    // Treat input as a comma-delimited list of breakpoints
    if (module_name.empty()) {
      module_name = "chrome.dll";
      DERROR("No module name provided. Using the default: \"chrome.dll\"\n");
    }

    breakpoint_list = BreakpointList(input_str, module_name, new_tag);
    skip_tag_update = true;
  }

  // Apply module name changes if needed
  if (!module_name.empty() && breakpoint_list.IsValid()) {
    if (replace_all_modules) {
      // Replace all module names
      breakpoint_list.ReplaceAllModuleNames(module_name);
    }
  }

  if (!skip_tag_update) {
    // Update tag if needed
    if (!new_tag.empty()) {
      breakpoint_list.SetTag((new_tag == "-") ? "" : new_tag);
    }
  }

  return breakpoint_list;
}

void SetBreakpointsInternal(const std::string& breakpoints_delimited,
                            const std::string& new_module_name,
                            const std::string& new_tag,
                            bool all_processes,
                            bool run_commands = true) {
  BreakpointList breakpoint_list = GetBreakpointListFromArgs(
      breakpoints_delimited, new_module_name, new_tag);

  if (!breakpoint_list.IsValid()) {
    DERROR("\nError: One or more breakpoints are invalid.\n");
    return;
  }

  if (run_commands) {
    // Update the history
    // Remove any identical breakpoint list
    std::vector<BreakpointList> new_list;
    for (const auto& bl : g_breakpoint_lists) {
      if (!bl.IsEqualTo(breakpoint_list)) {
        new_list.push_back(bl);
      }
    }

    // Add the current list to the top
    new_list.insert(new_list.begin(), breakpoint_list);
    g_breakpoint_lists = new_list;
    WriteBreakpointsToFile();
  }

  if (all_processes) {
    g_breakpoint_list = breakpoint_list;

    // Turn on child process debugging
    g_debug.control->Execute(DEBUG_OUTCTL_ALL_CLIENTS, ".childdbg 1",
                             DEBUG_EXECUTE_DEFAULT);
    g_debug.control->Execute(DEBUG_OUTCTL_ALL_CLIENTS, "sxn ibp",
                             DEBUG_EXECUTE_DEFAULT);
    g_debug.control->Execute(DEBUG_OUTCTL_ALL_CLIENTS, "sxn epr",
                             DEBUG_EXECUTE_DEFAULT);

    DOUT("\nSetting the following breakpoints for all processes:\n\n");
    DOUT("%s\n", breakpoint_list.ToLongString("\t").c_str());
    DOUT("\n");
  } else {
    std::string command_string = breakpoint_list.GetCombinedCommandString();

    DOUT("\nSetting the following breakpoints for the current process:\n\n");
    DOUT("%s\n", breakpoint_list.ToLongString("\t").c_str());
    DOUT("\t%s\n\n", command_string.c_str());

    if (run_commands) {
      // Execute the command string
      g_debug.control->Execute(DEBUG_OUTCTL_ALL_CLIENTS, command_string.c_str(),
                               DEBUG_EXECUTE_DEFAULT);
    }
  }

  if (!run_commands) {
    DOUT("\nDRY RUN. No commands executed and history has not been updated.\n");
  }
}

void ListBreakpoints(std::string search_term,
                     size_t count = 10,
                     const std::string& indent = "",
                     bool show_header = true) {
  // Allow both "s:" and plain text for search term.
  // This is more inline with the other functions.
  if (!search_term.empty() && search_term.substr(0, 2) == "s:") {
    search_term = search_term.substr(2);
  }

  auto filtered_lists = GetFilteredBreakpointLists(search_term);

  if (show_header) {
    DOUT("\n%sBreakpoints history:\n", indent.c_str());
  }

  for (size_t i = 0; i < filtered_lists.size(); i++) {
    DOUT("%s\t%u) %s\n", indent.c_str(), filtered_lists[i].first,
         filtered_lists[i].second.ToShortString().c_str());

    if (count > 0 && i == (count - 1)) {
      DOUT("%s\t... and %u more\n", indent.c_str(),
           filtered_lists.size() - count);
      break;
    }
  }

  DOUT("\n");
}

// Returns a sorted vector of indices based on the input string.
std::vector<size_t> GetIndicesFromSingleInput(const std::string& input_str) {
  std::vector<size_t> indices;

  if (std::all_of(input_str.begin(), input_str.end(), ::isdigit)) {
    // Single numeric index
    indices.push_back(std::stoi(input_str));
  } else if (std::regex_match(input_str,
                              std::regex("\\s*\\d+(\\s+\\d+|\\-\\d+)*\\s*"))) {
    // Space-separated list of indices
    indices = utils::GetIndicesFromString(input_str);
  } else if (input_str.substr(0, 2) == "s:") {
    // Search by text
    std::string search_term = utils::Trim(input_str.substr(2));
    if (search_term.empty()) {
      DERROR("No search term provided.\n");
      return indices;
    }

    auto filtered_lists = GetFilteredBreakpointLists(search_term);
    if (filtered_lists.empty()) {
      DERROR("\nNo breakpoints found matching: %s\n\n", search_term.c_str());
      return indices;
    }

    for (const auto& entry : filtered_lists) {
      indices.push_back(entry.first);
    }
  } else if (input_str.substr(0, 2) == "t:") {
    // Search by tag
    std::string tag = utils::Trim(input_str.substr(2));
    if (tag.empty()) {
      DERROR("No tag provided.\n");
      return indices;
    }

    for (size_t i = 0; i < g_breakpoint_lists.size(); i++) {
      if (g_breakpoint_lists[i].HasTagMatch(tag)) {
        indices.push_back(i);
      }
    }

    if (indices.empty()) {
      DERROR("\nNo breakpoints found with tag matching: %s\n\n", tag.c_str());
      return indices;
    }
  } else {
    DERROR("Error: Invalid input format.\n");
    return indices;
  }

  // Filter out invalid indices
  std::vector<size_t> valid_indices;
  for (const auto& idx : indices) {
    if (idx < 0 || idx >= g_breakpoint_lists.size()) {
      DERROR("Warning: Index %d is out of range and will be ignored.\n", idx);
    } else {
      valid_indices.push_back(idx);
    }
  }

  std::sort(valid_indices.begin(), valid_indices.end());
  return valid_indices;
}

HRESULT CALLBACK ListBreakpointsHistoryInternal(IDebugClient* client,
                                                const char* args) {
  if (args && strcmp(args, "?") == 0) {
    const char* help_text = R"(
ListBreakpointsHistory Usage:

This function displays the saved breakpoint lists from history.

Parameters:
- searchTerm: Optional filter to limit the displayed breakpoints:
  * null: Shows all breakpoint lists (default)
  * "s:term": Shows only breakpoint lists containing "term"
  * "searchTerm": Shows only breakpoint lists containing "searchTerm"
  * "?": Shows this help information

- count: Optional maximum number of breakpoint lists to display:
  * Default: 15
  * 0: Shows all matching breakpoint lists
  * n: Shows at most n breakpoint lists

Examples:
- !ListBreakpointsHistory - Show up to 15 most recent breakpoint lists from history
- !ListBreakpointsHistory 5 - Show only the first 5 breakpoint lists
- !ListBreakpointsHistory ReadFile - Show breakpoint lists containing "ReadFile"
- !ListBreakpointsHistory s:ReadFile - Show breakpoint lists containing "ReadFile"
- !ListBreakpointsHistory ReadFile 5 - Show breakpoint lists containing "ReadFile" (up to 5 shown)
- !ListBreakpointsHistory 0 - Show all breakpoint lists in history

Note: The indices shown can be used with other functions like !SetBreakpoints <index> or !RemoveBreakpointsFromHistory <index>.
)";
    DOUT("%s\n", help_text);
    return S_OK;
  }

  // Parse arguments
  std::string search_term;
  int count = 10;

  if (args) {
    std::vector<std::string> parsed_args = utils::ParseCommandLine(args);

    // If there is more than one argument return an error
    if (parsed_args.size() > 2) {
      DERROR("Error: Too many arguments provided.\n");
      return E_INVALIDARG;
    }

    if (!parsed_args.empty()) {
      // Check if the first argument is a number
      if (std::all_of(parsed_args[0].begin(), parsed_args[0].end(),
                      ::isdigit)) {
        count = std::stoi(parsed_args[0]);
      } else {
        // The first argument is the search term
        search_term = parsed_args[0];

        // Check to see if the second argument is a number
        if (parsed_args.size() > 1 &&
            std::all_of(parsed_args[1].begin(), parsed_args[1].end(),
                        ::isdigit)) {
          count = std::stoi(parsed_args[1]);
        }
      }
    }
  }

  ListBreakpoints(search_term, count);
  return S_OK;
}

HRESULT CALLBACK SetBreakpointsInternal(IDebugClient* client,
                                        const char* args) {
  if (args && strcmp(args, "?") == 0) {
    const char* helpText = R"(
SetBreakpoints Usage:

This function sets breakpoints in the current process only.

Parameters:
- breakpointsDelimited: A string of breakpoints separated by commas (,) or one of:
  * null: Uses the first breakpoint in history
  * Number: Index of breakpoint from history to use
  * "s:term": Searches breakpoint history for "term"
  * "t:tag": Finds breakpoints with matching tag
  * Space-separated numbers: Combines breakpoints from specified indices (e.g., '1 2 3')
  * Space-separated numbers with .n suffix: Uses specific breakpoints (e.g., '1.0 2.1 3')
  * 'Space-separated numbers + breakpoints': Combines breakpoints from history with new breakpoints
  * ".": Set breakpoint at the current source location
  * ".:line": Set breakpoint at specified line number in the current source file
  * "!": Dry-run mode - shows what would be done without executing commands
  * "?": Shows this help information

- newModuleName: The module to set breakpoints in
  * null: Uses default if creating new breakpoints, or original if from history
  * ".": Uses the original module name from history. Useful if only changing the tag.
  * "moduleName": Sets as default module name for breakpoints without one
  * "+moduleName": Replaces all module names with the specified module

- newTag: A descriptive tag for these breakpoints
  * null: Uses original tag if from history
  * "-": Uses empty string

Note: If an input argument contains a space then it needs to be
      enclosed in single quotes. For example, 'kernel32!ReadFile; kernel32!WriteFile'.

Examples:
- !SetBreakpoints - Use first breakpoint from history
- !SetBreakpoints 3 - Use breakpoint at index 3 from history
- !SetBreakpoints 3.1 - Use the second breakpoint at index 3 from history
- !SetBreakpoints ! 3 - Show what would be done for index 3 without executing commands
- !SetBreakpoints 3 tests.exe - Use breakpoint at index 3 from history and set default module name
- !SetBreakpoints 3 +tests.exe - Use breakpoint at index 3 from history and replace all module names
- !SetBreakpoints 3 . new_tag - Use breakpoint at index 3 from history and set a new tag
- !SetBreakpoints 3 . - - Use breakpoint at index 3 from history and remove the tag
- !SetBreakpoints '1 2 3' - Combine breakpoints from history indices 1, 2, and 3
- !SetBreakpoints '1.0 2.1' - Use first breakpoint from list 1 and second from list 2
- !SetBreakpoints '1.0 2.1' chrome.dll - Use first breakpoint from list 1 and second from list 2 and set the module name
- !SetBreakpoints s:ReadFile - Search history for "ReadFile"
- !SetBreakpoints t:file_operations - Find breakpoints with tag "file_operations"
- !SetBreakpoints kernel32!ReadFile kernel32.dll file_ops - Set new breakpoint with module name and tag
- !SetBreakpoints 'kernel32!ReadFile, kernel32!WriteFile' kernel32.dll file_ops - Set new breakpoints
- !SetBreakpoints '1 2.1 4 + chrome!ReadFile' - Combine breakpoints from history with a new breakpoint
- !SetBreakpoints '0 + ntdll!NtCreateFile, ntdll!NtReadFile' - Add new breakpoints to those from history index 0
- !SetBreakpoints . - Set breakpoint at the current source location
- !SetBreakpoints .:150 - Set breakpoint at line 150 in the current source file
- !SetBreakpoints . . debug_tag - Set tagged breakpoint at the current location if specified.

Note: Unlike !SetAllProcessesBreakpoints, this function only affects the current process
)";
    DOUT("%s\n", helpText);
    return S_OK;
  }

  // Parse arguments
  std::string breakpoints_delimited;
  std::string new_module_name;
  std::string new_tag;
  bool dry_run = false;

  if (args) {
    std::vector<std::string> parsed_args = utils::ParseCommandLine(args);

    // Check if the first argument is the dry run indicator
    if (!parsed_args.empty() && parsed_args[0] == "!") {
      dry_run = true;
      parsed_args.erase(parsed_args.begin());
    }

    // The first argument is the breakpoints list
    if (!parsed_args.empty()) {
      breakpoints_delimited = parsed_args[0];
    }

    // The second argument is the module name
    if (parsed_args.size() > 1) {
      new_module_name = parsed_args[1];
    }

    // The third argument is the tag
    if (parsed_args.size() > 2) {
      new_tag = parsed_args[2];
    }
  }

  SetBreakpointsInternal(breakpoints_delimited, new_module_name, new_tag, false,
                         !dry_run);
  return S_OK;
}

HRESULT CALLBACK SetAllProcessesBreakpointsInternal(IDebugClient* client,
                                                    const char* args) {
  if (args && strcmp(args, "?") == 0) {
    const char* help_text = R"(
SetAllProcessesBreakpoints Usage:

This function sets breakpoints that will be applied to all processes, including
child processes that are launched after the breakpoints are set.

Parameters:
- breakpointsDelimited: A string of breakpoints separated by commas (,) or one of:
  * null: Uses the first breakpoint in history
  * Number: Index of breakpoint from history to use
  * "s:term": Searches breakpoint history for "term"
  * "t:tag": Finds breakpoints with matching tag
  * Space-separated numbers: Combines breakpoints from specified indices (e.g., '1 2 3')
  * Space-separated numbers with .n suffix: Uses specific breakpoints (e.g., '1.0 2.1 3')
  * 'Space-separated numbers + breakpoints': Combines breakpoints from history with new breakpoints
  * "!": Dry-run mode - shows what would be done without executing commands
  * "?": Shows this help information

- newModuleName: The module to set breakpoints in
  * null: Uses default if creating new breakpoints, or original if from history
  * ".": Uses the original module name from history. Useful if only changing the tag.
  * "moduleName": Sets as default module name for breakpoints without one
  * "+moduleName": Replaces all module names with the specified module

- newTag: A descriptive tag for these breakpoints
  * null: Uses original tag if from history
  * "-": Uses empty string

Note: If an input argument contains a space then it needs to be
      enclosed in single quotes. For example, 'kernel32!ReadFile; kernel32!WriteFile'.

Examples:
- !SetAllProcessesBreakpoints - Use first breakpoint from history
- !SetAllProcessesBreakpoints 3 - Use breakpoint at index 3 from history
- !SetAllProcessesBreakpoints 3.1 - Use the second breakpoint at index 3 from history
- !SetAllProcessesBreakpoints ! 3 - Show what would be done for index 3 without executing commands
- !SetAllProcessesBreakpoints 3 tests.exe - Use breakpoint at index 3 from history and set default module name
- !SetAllProcessesBreakpoints 3 +tests.exe - Use breakpoint at index 3 from history and replace all module names
- !SetAllProcessesBreakpoints 3 . new_tag - Use breakpoint at index 3 from history and set a new tag
- !SetAllProcessesBreakpoints 3 . - - Use breakpoint at index 3 from history and remove the tag
- !SetAllProcessesBreakpoints '1 2 3' - Combine breakpoints from history indices 1, 2, and 3
- !SetAllProcessesBreakpoints '1.0 2.1' - Use first breakpoint from list 1 and second from list 2
- !SetAllProcessesBreakpoints '1.0 2.1' chrome.dll - Use first breakpoint from list 1 and second from list 2 and set the module name
- !SetAllProcessesBreakpoints s:ReadFile - Search history for "ReadFile"
- !SetAllProcessesBreakpoints t:file_operations - Find breakpoints with tag "file_operations"
- !SetAllProcessesBreakpoints kernel32!ReadFile kernel32.dll file_ops - Set new breakpoint with module name and tag
- !SetAllProcessesBreakpoints 'kernel32!ReadFile, kernel32!WriteFile' kernel32.dll file_ops - Set new breakpoints
- !SetAllProcessesBreakpoints '1 2.1 4 + chrome!ReadFile' - Combine breakpoints from history with a new breakpoint
- !SetAllProcessesBreakpoints '0 + ntdll!NtCreateFile, ntdll!NtReadFile' - Add new breakpoints to those from history index 0
)";
    DOUT("%s\n", help_text);
    return S_OK;
  }

  // Parse arguments
  std::string breakpoints_delimited;
  std::string new_module_name;
  std::string new_tag;
  bool dry_run = false;

  if (args) {
    std::vector<std::string> parsed_args = utils::ParseCommandLine(args);

    // Check if the first argument is the dry run indicator
    if (!parsed_args.empty() && parsed_args[0] == "!") {
      dry_run = true;
      parsed_args.erase(parsed_args.begin());
    }

    // The first argument is the breakpoints list
    if (!parsed_args.empty()) {
      breakpoints_delimited = parsed_args[0];
    }

    // The second argument is the module name
    if (parsed_args.size() > 1) {
      new_module_name = parsed_args[1];
    }

    // The third argument is the tag
    if (parsed_args.size() > 2) {
      new_tag = parsed_args[2];
    }
  }

  // Register event callbacks if not already done
  if (!g_event_callbacks) {
    g_event_callbacks = new EventCallbacks();
    HRESULT hr = g_debug.client->SetEventCallbacks(g_event_callbacks);
    if (FAILED(hr)) {
      g_event_callbacks->Release();
      g_event_callbacks = nullptr;
      DERROR("Failed to set event callbacks: 0x%08X\n", hr);
      return hr;
    }
  }

  SetBreakpointsInternal(breakpoints_delimited, new_module_name, new_tag, true,
                         !dry_run);
  return S_OK;
}

HRESULT CALLBACK RemoveBreakpointsFromHistoryInternal(IDebugClient* client,
                                                      const char* args) {
  if (args && strcmp(args, "?") == 0) {
    const char* help_text = R"(
RemoveBreakpointsFromHistory Usage:

This function removes specific breakpoint lists from the saved history.

Parameter:
- input: Specifies which breakpoint lists to remove:
  * Number: Index of a single breakpoint list to remove
  * "0 1 2": Space-separated string of indices to remove multiple lists
  * "0-2": Range notation to specify consecutive indices (equivalent to "0 1 2")
  * "1-3 5 7-9": Mixed format combining ranges and individual indices
  * "s:term": Removes all breakpoint lists matching the search term
  * "t:tag": Removes all breakpoint lists with tags matching the given tag
  * "?": Shows this help information

Examples:
- !RemoveBreakpointsFromHistory 0 - Remove the breakpoint list at index 0
- !RemoveBreakpointsFromHistory '3 5 7' - Remove breakpoint lists at indices 3, 5, and 7
- !RemoveBreakpointsFromHistory 1-5 - Remove breakpoint lists at indices 1, 2, 3, 4, and 5
- !RemoveBreakpointsFromHistory '1-3 5 7-9' - Remove breakpoint lists at indices 1, 2, 3, 5, 7, 8, and 9
- !RemoveBreakpointsFromHistory s:ReadFile - Remove all breakpoint lists containing "ReadFile"
- !RemoveBreakpointsFromHistory t:file_operations - Remove all breakpoint lists with tags matching "file_operations"

Note: This operation cannot be undone.
)";
    DOUT("%s\n", help_text);
    return S_OK;
  }

  if (g_breakpoint_lists.empty()) {
    DERROR("No breakpoint history available to remove from.\n");
    return E_FAIL;
  }

  auto parsed_args = utils::ParseCommandLine(args);
  if (parsed_args.empty()) {
    DERROR("Error: invalid input format.\n");
    return E_INVALIDARG;
  }
  if (parsed_args.size() > 1) {
    DERROR("Error: too many arguments provided.\n");
    return E_INVALIDARG;
  }

  auto valid_indices = GetIndicesFromSingleInput(parsed_args[0]);
  if (valid_indices.empty()) {
    DERROR("No valid indices to remove.\n");
    return E_FAIL;
  }

  // Display what will be removed
  DOUT("\nRemoving the following breakpoints from history:\n");
  for (const auto& idx : valid_indices) {
    DOUT("\t%u) %s\n", idx, g_breakpoint_lists[idx].ToShortString().c_str());
  }

  // Remove the breakpoints
  std::vector<BreakpointList> new_list;
  for (size_t i = 0; i < g_breakpoint_lists.size(); i++) {
    if (std::find(valid_indices.begin(), valid_indices.end(), i) ==
        valid_indices.end()) {
      new_list.push_back(g_breakpoint_lists[i]);
    }
  }
  g_breakpoint_lists = new_list;

  WriteBreakpointsToFile();

  DOUT("\nSuccessfully removed %u breakpoint list(s) from history.\n",
       valid_indices.size());
  return S_OK;
}

HRESULT CALLBACK SetBreakpointsHistoryTagsInternal(IDebugClient* client,
                                                   const char* args) {
  if (args && strcmp(args, "?") == 0) {
    const char* help_text = R"(
SetBreakpointHistoryTags Usage:

This function updates the tags of specific breakpoint lists in the saved history.

Parameters:
- input: Specifies which breakpoint lists to update:
  * Number: Index of a single breakpoint list to update
  * "0 1 2": Space-separated string of indices to update multiple lists
  * "0-2": Range notation to specify consecutive indices (equivalent to "0 1 2")
  * "1-3 5 7-9": Mixed format combining ranges and individual indices
  * "s:term": Updates all breakpoint lists matching the search term
  * "t:tag": Updates all breakpoint lists with tags matching the given tag
  * "?": Shows this help information

- newTag: The new tag to assign to the selected breakpoint lists
  * String: Any text to use as the new tag
  * "-": Removes existing tags (sets to empty string)

Examples:
- !SetBreakpointsHistoryTags 0 new_tag - Update the tag of breakpoint list at index 0
- !SetBreakpointsHistoryTags '3 5 7' file_ops - Update tags for breakpoint lists at indices 3, 5, and 7
- !SetBreakpointsHistoryTags '1-5' file_ops - Update tags for breakpoint lists at indices 1 through 5
- !SetBreakpointsHistoryTags '1-3 5 7-9' file_ops - Update tags for the specified range of indices
- !SetBreakpointsHistoryTags s:ReadFile io_operations - Update tags for all lists containing "ReadFile"
- !SetBreakpointsHistoryTags t:old_tag new_tag - Update all lists with tags matching "old_tag"
- !SetBreakpointsHistoryTags '0 1' - - Remove tags from breakpoint lists at indices 0 and 1

Note: Changes will be saved to disk immediately.
)";
    DOUT("%s\n", help_text);
    return S_OK;
  }

  if (g_breakpoint_lists.empty()) {
    DERROR("No breakpoint history available to update.\n");
    return E_FAIL;
  }

  // Parse arguments
  std::vector<std::string> parsed_args = utils::ParseCommandLine(args);
  if (parsed_args.size() < 2) {
    DERROR("Error: missing parameters. Expected input and newTag.\n");
    return E_INVALIDARG;
  }

  std::string input_str = parsed_args[0];
  std::string new_tag = parsed_args[1];

  auto indices_to_update = GetIndicesFromSingleInput(input_str);

  if (indices_to_update.empty()) {
    DERROR("No valid indices to update.\n");
    return E_FAIL;
  }

  // Display what breakpoints will be updated
  DOUT("\nUpdating tags for the following breakpoints:\n");
  for (const auto& idx : indices_to_update) {
    DOUT("\t%u) %s\n", idx, g_breakpoint_lists[idx].ToShortString().c_str());
  }

  if (new_tag == "-") {
    new_tag = "";
  }

  // Update the tags
  for (const auto& idx : indices_to_update) {
    g_breakpoint_lists[idx].SetTag(new_tag);
  }

  WriteBreakpointsToFile();

  DOUT("\nSuccessfully updated tags for %u breakpoint list(s).\n",
       indices_to_update.size());
  DOUT("New tag: \"%s\"\n\n", new_tag.c_str());

  // Show updated breakpoint lists
  DOUT("Updated breakpoints:\n");
  for (const auto& idx : indices_to_update) {
    DOUT("\t%u) %s\n", idx, g_breakpoint_lists[idx].ToShortString().c_str());
  }

  return S_OK;
}

HRESULT CALLBACK UpdateBreakpointLineNumberInternal(IDebugClient* client,
                                                    const char* args) {
  if (args && strcmp(args, "?") == 0) {
    const char* help_text = R"(
UpdateBreakpointLineNumber Usage:

This command updates the line number of a specific breakpoint in the history.

Parameters:
- input: Specifies which breakpoint to update:
  * [+]             : Optional first argument. If present, adds the updated breakpoint as a new entry.
                      Without '+', updates the breakpoint list in place.
  * number          : Index of a breakpoint list in history (updates first line-number breakpoint found)
  * number1.number2 : Updates the specific breakpoint at index number2 in the list at index number1
  * ?               : Shows this help information

- newLineNumber: The new line number to set (must be a positive integer)

Examples:
- !UpdateBreakpointLineNumber 0 150 - Update in place the first source:line breakpoint in list 0 to line 150
- !UpdateBreakpointLineNumber + 0 150 - Update the first source:line breakpoint in list 0 to line 150 and add as new entry
- !UpdateBreakpointLineNumber 2.1 275 - Update in place the second breakpoint in list 2 to line 275
- !UpdateBreakpointLineNumber + 2.1 275 - Update the second breakpoint and add as new entry

Notes:
- This only works for breakpoints that include line numbers (e.g., "module!file.cpp:123")
- With '+': The updated breakpoint list will be saved to history as a new entry
- Without '+': The breakpoint list will be updated in place
- The updated breakpoint will NOT be automatically set in the debugger
)";
    DOUT("%s\n", help_text);
    return S_OK;
  }

  if (g_breakpoint_lists.empty()) {
    DERROR("No breakpoint history available to update.\n");
    return E_FAIL;
  }

  std::vector<std::string> parsed_args = utils::ParseCommandLine(args);
  if (parsed_args.empty()) {
    DERROR("Error: missing parameters. Expected input and newLineNumber.\n");
    return E_INVALIDARG;
  }

  // Check if we should add as new entry or update in place
  bool add_as_new_entry = false;
  if (parsed_args[0] == "+") {
    add_as_new_entry = true;
    parsed_args.erase(parsed_args.begin());  // Remove the plus sign
  }

  // Now we need at least the index and line number
  if (parsed_args.size() < 2) {
    DERROR("Error: missing parameters. Expected input and newLineNumber.\n");
    return E_INVALIDARG;
  }

  std::string input_str = parsed_args[0];
  std::string line_number_str = parsed_args[1];

  // Parse the new line number
  if (!utils::IsWholeNumber(line_number_str)) {
    DERROR("Error: Line number must be a positive integer.\n");
    return E_INVALIDARG;
  }
  size_t new_line_number = std::stoul(line_number_str);

  size_t list_index;
  std::optional<size_t> bp_index;

  if (!utils::ParseNumberOrDottedPair(input_str, list_index, bp_index)) {
    DERROR("Error: Invalid input format: %s\n", input_str.c_str());
    return E_INVALIDARG;
  }

  // Validate list index
  if (list_index >= g_breakpoint_lists.size()) {
    DERROR("Error: List index %u is out of range (0-%u).\n", list_index,
           g_breakpoint_lists.size() - 1);
    return E_INVALIDARG;
  }

  // Create a copy of the breakpoint list to modify
  BreakpointList breakpoint_list = g_breakpoint_lists[list_index];
  bool success = false;

  if (bp_index.has_value()) {
    if (bp_index.value() >= breakpoint_list.GetBreakpointsCount()) {
      DERROR("Error: Invalid breakpoint index %u for list index %u.\n",
             bp_index.value(), list_index);
      return E_INVALIDARG;
    }
    success =
        breakpoint_list.UpdateLineNumber(bp_index.value(), new_line_number);
  } else {
    // Try to update the first applicable breakpoint
    success = breakpoint_list.UpdateFirstLineNumber(new_line_number);
  }

  if (!success) {
    DERROR("Error: No breakpoints with line numbers found in list %u.\n",
           list_index);
    return E_FAIL;
  }

  if (add_as_new_entry) {
    // Add the updated list to history as a new entry
    // Remove any identical breakpoint list
    std::vector<BreakpointList> new_list;
    for (const auto& bl : g_breakpoint_lists) {
      if (!bl.IsEqualTo(breakpoint_list)) {
        new_list.push_back(bl);
      }
    }

    // Add the updated list to the top
    new_list.insert(new_list.begin(), breakpoint_list);
    g_breakpoint_lists = new_list;

    DOUT(
        "\nSuccessfully updated line number and saved to history as index 0.\n");
  } else {
    // Update the breakpoint list in place
    g_breakpoint_lists[list_index] = breakpoint_list;

    DOUT("\nSuccessfully updated line number in place at index %u.\n",
         list_index);
  }

  WriteBreakpointsToFile();

  DOUT("Updated breakpoint list: %s\n\n",
       breakpoint_list.ToShortString().c_str());

  return S_OK;
}

HRESULT CALLBACK SetBreakpointListsFileInternal(IDebugClient* client,
                                                const char* args) {
  if (args && strcmp(args, "?") == 0) {
    const char* help_text = R"(
SetBreakpointListsFile Usage:

This function sets the path for the breakpoints history file and reloads the breakpoints from the new location.

Parameter:
- filePath: The full path to the breakpoints history JSON file
  * Must be a valid file path
  * The directory must exist (the file will be created if it doesn't exist)
  * "?": Shows this help information

Examples:
- !SetBreakpointListsFile C:\Debugger\my_breakpoints.json - Set a custom breakpoints file
- !SetBreakpointListsFile D:\Projects\debug\breakpoints_history.json - Use project-specific breakpoints

Notes:
- The path is not persisted between debugging sessions
- If the file doesn't exist, it will be created when breakpoints are saved
- If the file exists but is invalid, the breakpoints list will be cleared
- The default location is in the same directory as the extension DLL
)";
    DOUT("%s\n", help_text);
    return S_OK;
  }

  if (!args || strlen(args) == 0) {
    DERROR("Error: No file path provided.\n");
    return E_INVALIDARG;
  }

  // Parse the file path argument
  std::vector<std::string> parsed_args = utils::ParseCommandLine(args);
  if (parsed_args.empty()) {
    DERROR("Error: Invalid file path.\n");
    return E_INVALIDARG;
  }

  if (parsed_args.size() > 1) {
    DERROR("Error: Too many arguments. Expected a single file path.\n");
    return E_INVALIDARG;
  }

  std::string new_file_path = parsed_args[0];

  // Basic validation - check if the directory exists
  std::string directory = new_file_path;
  size_t last_separator = directory.find_last_of("\\/");
  if (last_separator != std::string::npos) {
    directory = directory.substr(0, last_separator);

    DWORD attrib = GetFileAttributesA(directory.c_str());
    if (attrib == INVALID_FILE_ATTRIBUTES ||
        !(attrib & FILE_ATTRIBUTE_DIRECTORY)) {
      DERROR("Error: Directory does not exist: %s\n", directory.c_str());
      return E_INVALIDARG;
    }
  }

  // Set the new file path
  g_breakpoint_lists_file = new_file_path;

  DOUT("Setting breakpoints history file to: %s\n",
       g_breakpoint_lists_file.c_str());

  // Reinitialize breakpoints with the new file
  InitializeBreakpoints();

  DOUT("Loaded %u breakpoint list(s) from the new file.\n",
       g_breakpoint_lists.size());
  return S_OK;
}

// Initialize the extension
HRESULT CALLBACK DebugExtensionInitializeInternal(PULONG version,
                                                  PULONG flags) {
  *version = DEBUG_EXTENSION_VERSION(1, 0);
  *flags = 0;

  HRESULT hr = utils::InitializeDebugInterfaces(&g_debug);
  if (FAILED(hr)) {
    return hr;
  }
  return S_OK;
}

HRESULT CALLBACK DebugExtensionUninitializeInternal() {
  // Clean up the event handler
  if (g_event_callbacks) {
    g_debug.client->SetEventCallbacks(nullptr);
    g_event_callbacks->Release();
    g_event_callbacks = nullptr;
  }

  return utils::UninitializeDebugInterfaces(&g_debug);
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

__declspec(dllexport) HRESULT CALLBACK
ListBreakpointsHistory(IDebugClient* client, const char* args) {
  return ListBreakpointsHistoryInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK SetBreakpoints(IDebugClient* client,
                                                      const char* args) {
  return SetBreakpointsInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK
SetAllProcessesBreakpoints(IDebugClient* client, const char* args) {
  return SetAllProcessesBreakpointsInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK
RemoveBreakpointsFromHistory(IDebugClient* client, const char* args) {
  return RemoveBreakpointsFromHistoryInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK
SetBreakpointsHistoryTags(IDebugClient* client, const char* args) {
  return SetBreakpointsHistoryTagsInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK
UpdateBreakpointLineNumber(IDebugClient* client, const char* args) {
  return UpdateBreakpointLineNumberInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK
SetBreakpointListsFile(IDebugClient* client, const char* args) {
  return SetBreakpointListsFileInternal(client, args);
}
}

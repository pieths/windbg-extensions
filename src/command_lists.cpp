// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#define NOMINMAX  // Prevent windows.h from defining min/max macros
#include <dbgeng.h>
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <string>
#include <tuple>
#include <vector>

#include "command_list.h"
#include "json.hpp"
#include "utils.h"

using JSON = nlohmann::json;

// WinDbg extension globals
utils::DebugInterfaces g_debug;

// Global variables to store command lists
std::vector<CommandList> g_command_lists;
std::string g_command_lists_file;
CommandList* g_current_command_list = nullptr;
std::string g_commands_file;
bool g_enable_break_event_handler_output = true;

void InitializeCommandLists() {
  if (g_command_lists_file.empty()) {
    g_command_lists_file =
        // Use the same location as the DLL if no path was set
        utils::GetCurrentExtensionDir() + "\\command_lists.json";
  }

  std::ifstream file(g_command_lists_file);
  if (file.is_open()) {
    try {
      JSON json;
      file >> json;
      file.close();

      g_command_lists.clear();
      if (json.is_array()) {
        for (const auto& item : json) {
          CommandList cmd_list = CommandList::FromJson(item);
          g_command_lists.push_back(cmd_list);
        }
      }
    } catch (const std::exception& e) {
      DERROR("Error loading command lists: %s\n", e.what());
    }
  } else {
    g_command_lists.clear();
  }
}

void WriteCommandListsToFile() {
  if (g_command_lists.empty()) {
    DOUT("No command lists to save.\n");
    return;
  }

  try {
    JSON json = JSON::array();
    for (const auto& cmd_list : g_command_lists) {
      json.push_back(cmd_list.ToJson());
    }

    std::ofstream file(g_command_lists_file);
    if (file.is_open()) {
      file << json.dump(4);
      file.close();
    } else {
      DERROR("Failed to open file for writing: %s\n",
             g_command_lists_file.c_str());
    }
  } catch (const std::exception& e) {
    DERROR("Error saving command lists: %s\n", e.what());
  }
}

std::vector<std::string> GetCommandsFromFile() {
  std::vector<std::string> commands;
  std::ifstream file(g_commands_file);

  if (!file.is_open()) {
    DERROR("Failed to open commands file: %s\n", g_commands_file.c_str());
    return commands;
  }

  std::string line;
  bool ignore_lines = false;

  while (std::getline(file, line)) {
    line = utils::Trim(line);

    if (line == "!StopCommandListRecording") {
      break;
    } else if (line == "!PauseCommandListRecording") {
      ignore_lines = true;
      continue;
    } else if (line == "!ResumeCommandListRecording") {
      ignore_lines = false;
      continue;
    } else if (!ignore_lines) {
      commands.push_back(line);
    }
  }

  file.close();
  return commands;
}

std::vector<std::pair<size_t, CommandList*>> GetClosestCommandLists(
    int source_line,
    const std::string& source_file) {
  std::vector<std::tuple<int, size_t, CommandList*>> sorted_lists;

  for (size_t i = 0; i < g_command_lists.size(); i++) {
    auto& cl = g_command_lists[i];
    if (cl.GetSourceFile() == source_file) {
      int distance = std::abs(cl.GetSourceLine() - source_line);
      sorted_lists.push_back({distance, i, &cl});
    }
  }

  std::sort(sorted_lists.begin(), sorted_lists.end(),
            [](const auto& a, const auto& b) {
              return std::get<0>(a) < std::get<0>(b);
            });

  std::vector<std::pair<size_t, CommandList*>> result;
  size_t count = std::min(sorted_lists.size(), static_cast<size_t>(5));
  for (size_t i = 0; i < count; i++) {
    result.push_back(
        {std::get<1>(sorted_lists[i]), std::get<2>(sorted_lists[i])});
  }

  return result;
}

void RegisterBreakEventHandler() {
  // Use the existing !AddBreakCommand to register our handler
  std::string command = "!AddBreakCommand !ShowNearbyCommandLists";
  g_debug.control->Execute(DEBUG_OUTCTL_THIS_CLIENT, command.c_str(),
                           DEBUG_EXECUTE_DEFAULT);
}

void UnregisterBreakEventHandler() {
  // Find and remove our break command
  std::string output = utils::ExecuteCommand(&g_debug, "!ListBreakCommands");

  // Parse the output to find our command's index
  std::istringstream stream(output);
  std::string line;
  int index = -1;

  while (std::getline(stream, line)) {
    if (line.find("!ShowNearbyCommandLists") != std::string::npos) {
      // Extract the index from the line (format: "\t0) command")
      // Find the first digit in the line
      size_t first_digit_pos = line.find_first_of("0123456789");
      if (first_digit_pos != std::string::npos) {
        // Find where the number ends
        size_t end_pos = line.find_first_not_of("0123456789", first_digit_pos);

        // Extract the number substring
        std::string index_str;
        if (end_pos != std::string::npos) {
          index_str = line.substr(first_digit_pos, end_pos - first_digit_pos);
        } else {
          // Number goes to end of string
          index_str = line.substr(first_digit_pos);
        }

        index = std::stoi(index_str);
        break;
      }
    }
  }

  if (index >= 0) {
    std::string clear_command = "!RemoveBreakCommands " + std::to_string(index);
    g_debug.control->Execute(DEBUG_OUTCTL_THIS_CLIENT, clear_command.c_str(),
                             DEBUG_EXECUTE_DEFAULT);
  }
}

CommandList* FindCommandList(const std::string& name_or_line) {
  if (name_or_line.empty()) {
    // Find closest command list to current location
    utils::SourceInfo source_info = utils::GetCurrentSourceInfo(&g_debug);
    if (!source_info.is_valid) {
      return nullptr;
    }

    auto closest_lists =
        GetClosestCommandLists(source_info.line, source_info.full_path);
    return closest_lists.empty() ? nullptr : closest_lists[0].second;
  }

  // Check if it's a line number option (.:number)
  if (name_or_line.length() > 2 && name_or_line[0] == '.' &&
      name_or_line[1] == ':') {
    std::string line_str = name_or_line.substr(2);
    if (utils::IsWholeNumber(line_str)) {
      int line = std::stoi(line_str);
      utils::SourceInfo source_info = utils::GetCurrentSourceInfo(&g_debug);
      if (!source_info.is_valid) {
        return nullptr;
      }

      // Find first command list at specified line in current file
      for (auto& cmd_list : g_command_lists) {
        if (cmd_list.GetSourceFile() == source_info.full_path &&
            cmd_list.GetSourceLine() == line) {
          return &cmd_list;
        }
      }
      return nullptr;
    }
  }

  // Check if it's a plain index number
  if (utils::IsWholeNumber(name_or_line)) {
    size_t index = std::stoul(name_or_line);
    if (index < g_command_lists.size()) {
      return &g_command_lists[index];
    }
    return nullptr;
  }

  // Otherwise, find by name
  for (auto& cmd_list : g_command_lists) {
    if (utils::ContainsCI(cmd_list.GetName(), name_or_line)) {
      return &cmd_list;
    }
  }

  return nullptr;
}

bool RunCommandList(const CommandList* cmd_list) {
  g_enable_break_event_handler_output = false;
  bool error = false;
  std::string last_command;

  for (const auto& command : cmd_list->GetCommands()) {
    std::string command_to_execute;

    if (command.empty()) {
      // Use the previous command if current command is empty
      if (last_command.empty()) {
        // Skip if there's no previous command yet
        continue;
      }
      command_to_execute = last_command;
      DOUT("Repeating previous command: %s\n", command_to_execute.c_str());
    } else {
      command_to_execute = command;
      last_command = command;  // Update last command
      DOUT("Executing command: %s\n", command_to_execute.c_str());
    }

    HRESULT hr = g_debug.control->Execute(DEBUG_OUTCTL_ALL_CLIENTS,
                                          command_to_execute.c_str(),
                                          DEBUG_EXECUTE_DEFAULT);

    if (FAILED(hr)) {
      DERROR("Error executing command: %s\n", command_to_execute.c_str());
      DERROR("Stopping command list execution.\n");
      error = true;
      break;
    }

    // Wait for the debugger to settle after execution commands
    ULONG status;
    hr = g_debug.control->GetExecutionStatus(&status);
    if (FAILED(hr)) {
      DERROR("Failed to get execution status.\n");
      error = true;
      break;
    }

    // If the command started execution (p, t, g, etc.), wait for it to complete
    if (status == DEBUG_STATUS_GO || status == DEBUG_STATUS_GO_HANDLED ||
        status == DEBUG_STATUS_GO_NOT_HANDLED ||
        status == DEBUG_STATUS_STEP_INTO || status == DEBUG_STATUS_STEP_OVER ||
        status == DEBUG_STATUS_STEP_BRANCH) {
      // Wait for a total of 10 seconds for the target to break
      hr = g_debug.control->WaitForEvent(DEBUG_WAIT_DEFAULT, 10000);
      if (FAILED(hr)) {
        DERROR("Failed waiting for break event.\n");
        error = true;
        break;
      }

      // Check status again after wait
      hr = g_debug.control->GetExecutionStatus(&status);
      if (FAILED(hr)) {
        DERROR("Failed to get execution status after wait.\n");
        error = true;
        break;
      }
    }

    // Now check if we should continue with more commands
    if (status != DEBUG_STATUS_BREAK) {
      DOUT(
          "Target is not at break. Status: %u. Stopping command list execution.\n",
          status);
      error = true;
      break;
    }
  }

  g_enable_break_event_handler_output = true;
  return !error;
}

HRESULT CALLBACK ShowNearbyCommandListsInternal(IDebugClient* client,
                                                const char* args) {
  if (args && args[0] == '?' && args[1] == '\0') {
    DOUT(
        "ShowNearbyCommandLists - Show command lists near the current source location\n\n"
        "Usage: !ShowNearbyCommandLists\n\n"
        "This command is automatically called on break events when registered.\n");
    return S_OK;
  }

  if (!g_enable_break_event_handler_output) {
    return S_OK;
  }

  utils::SourceInfo source_info = utils::GetCurrentSourceInfo(&g_debug);
  if (!source_info.is_valid) {
    return S_OK;
  }

  auto command_lists =
      GetClosestCommandLists(source_info.line, source_info.full_path);

  if (!command_lists.empty()) {
    std::string output = "Command lists: ";
    for (const auto& cmd_list_pair : command_lists) {
      output += cmd_list_pair.second->ToShortString(
                    static_cast<int>(cmd_list_pair.first)) +
                " ";
    }
    DOUT("%s\n", output.c_str());
  }

  // If any command is set for auto-run in the list then
  // run the first one if the source context matches
  for (const auto& cmd_list_pair : command_lists) {
    const CommandList* cmd_list = cmd_list_pair.second;
    if (cmd_list->IsAutoRun()) {
      // Check if source line, file, and context all match
      if (cmd_list->GetSourceLine() == source_info.line &&
          cmd_list->GetSourceFile() == source_info.full_path &&
          cmd_list->GetSourceContext() == source_info.source_context) {
        DOUT("Running auto-run command list: %s\n",
             cmd_list->ToMediumString().c_str());
        if (!RunCommandList(cmd_list)) {
          DERROR("Failed to run command list: %s\n",
                 cmd_list->ToMediumString().c_str());
        }
        return S_OK;  // Stop after running the first auto-run command list
      }
    }
  }

  return S_OK;
}

HRESULT CALLBACK StartCommandListRecordingInternal(IDebugClient* client,
                                                   const char* args) {
  if (args && args[0] == '?' && args[1] == '\0') {
    DOUT(
        "StartCommandListRecording - Start recording a command list\n\n"
        "Usage: !StartCommandListRecording [name] [description]\n\n"
        "  [name]        - Optional name for the command list\n"
        "  [description] - Optional description for the command list\n\n"
        "Examples:\n"
        "  !StartCommandListRecording                            - Start recording without name or description\n"
        "  !StartCommandListRecording MyList                     - Start recording with name 'MyList'\n"
        "  !StartCommandListRecording MyList 'My test commands'  - Start recording with name and description\n\n"
        "Note: Use single quotes around the description if it contains spaces.\n"
        "Use !StopCommandListRecording to finish recording.\n");
    return S_OK;
  }

  std::string name;
  std::string description;

  if (args && *args) {
    std::vector<std::string> parsed_args = utils::ParseCommandLine(args);

    if (!parsed_args.empty()) {
      name = parsed_args[0];

      // Check if name starts with a number
      if (!name.empty() && std::isdigit(name[0])) {
        DERROR("Command list name cannot start with a number.\n");
        return E_INVALIDARG;
      }
    }

    if (parsed_args.size() > 1) {
      description = parsed_args[1];
    }

    if (parsed_args.size() > 2) {
      DERROR(
          "Too many arguments. Expected at most 2 arguments (name and description).\n");
      return E_INVALIDARG;
    }
  }

  if (g_current_command_list) {
    DERROR(
        "A command list is already being recorded. Use !StopCommandListRecording before recording another list.\n");
    return E_FAIL;
  }

  utils::SourceInfo source_info = utils::GetCurrentSourceInfo(&g_debug);
  if (!source_info.is_valid) {
    DERROR("Could not get current source information.\n");
    return E_FAIL;
  }

  g_current_command_list =
      new CommandList({}, source_info.line, source_info.full_path, name,
                      description, source_info.source_context);

  if (g_commands_file.empty()) {
    g_commands_file = utils::GetCurrentExtensionDir() + "\\commands_tmp.txt";
  }

  // Start command logging
  std::string log_command = "!StartCommandLogging " + g_commands_file;
  g_debug.control->Execute(DEBUG_OUTCTL_ALL_CLIENTS, log_command.c_str(),
                           DEBUG_EXECUTE_DEFAULT);
  return S_OK;
}

HRESULT CALLBACK StopCommandListRecordingInternal(IDebugClient* client,
                                                  const char* args) {
  if (args && args[0] == '?' && args[1] == '\0') {
    DOUT(
        "StopCommandListRecording - Stop recording the current command list\n\n"
        "Usage: !StopCommandListRecording\n\n"
        "Stops recording and saves the command list.\n");
    return S_OK;
  }

  if (!g_current_command_list) {
    DERROR(
        "No command list is being recorded. Use !StartCommandListRecording to start recording a command list.\n");
    return E_FAIL;
  }

  // Stop command logging
  g_debug.control->Execute(DEBUG_OUTCTL_ALL_CLIENTS, "!StopCommandLogging",
                           DEBUG_EXECUTE_DEFAULT);

  // Read commands from file
  std::vector<std::string> commands = GetCommandsFromFile();

  if (!commands.empty()) {
    g_current_command_list->SetCommands(commands);
    g_command_lists.push_back(*g_current_command_list);

    // Delete the temporary commands file
    std::remove(g_commands_file.c_str());

    int index = static_cast<int>(g_command_lists.size() - 1);

    WriteCommandListsToFile();
    DOUT("Command list saved: [%u] %s\n", index,
         g_current_command_list->ToMediumString().c_str());
  }

  delete g_current_command_list;
  g_current_command_list = nullptr;
  return S_OK;
}

HRESULT CALLBACK PauseCommandListRecordingInternal(IDebugClient* client,
                                                   const char* args) {
  if (args && args[0] == '?' && args[1] == '\0') {
    DOUT(
        "PauseCommandListRecording - Pause recording commands\n\n"
        "Usage: !PauseCommandListRecording\n\n"
        "Commands executed after this will not be included in the recording.\n"
        "Use !ResumeCommandListRecording to continue.\n");
    return S_OK;
  }

  // This command is a marker in the log file
  // The actual pausing logic is handled in GetCommandsFromFile()
  return S_OK;
}

HRESULT CALLBACK ResumeCommandListRecordingInternal(IDebugClient* client,
                                                    const char* args) {
  if (args && args[0] == '?' && args[1] == '\0') {
    DOUT(
        "ResumeCommandListRecording - Resume recording commands\n\n"
        "Usage: !ResumeCommandListRecording\n\n"
        "Resumes command recording after !PauseCommandListRecording.\n");
    return S_OK;
  }

  // This command is a marker in the log file
  // The actual resuming logic is handled in GetCommandsFromFile()
  return S_OK;
}

HRESULT CALLBACK RunCommandListInternal(IDebugClient* client,
                                        const char* args) {
  if (args && args[0] == '?' && args[1] == '\0') {
    DOUT(
        "RunCommandList - Execute a recorded command list\n\n"
        "Usage: !RunCommandList [option]\n\n"
        "Options:\n"
        "  (no args)      - Run the closest command list to current line\n"
        "  name           - Run the first command list with matching name\n"
        "  number         - Run command list at index 'number'\n"
        "  .:number       - Run the first command list at line 'number' in current file\n\n"
        "Examples:\n"
        "  !RunCommandList          - Run closest command list to current line\n"
        "  !RunCommandList MyList   - Run command list with name containing 'MyList'\n"
        "  !RunCommandList 0        - Run command list at index 0\n"
        "  !RunCommandList .:123    - Run command list at line 123 in current file\n");
    return S_OK;
  }

  std::string name_or_line;
  if (args && *args) {
    name_or_line = utils::Trim(args);
  }

  CommandList* cmd_list = FindCommandList(name_or_line);
  if (!cmd_list) {
    DERROR("No command list found.\n");
    return E_FAIL;
  }

  DOUT("Running command list: %s\n", cmd_list->ToMediumString().c_str());

  // Run the command list
  if (!RunCommandList(cmd_list)) {
    DERROR("Failed to run command list: %s\n",
           cmd_list->ToMediumString().c_str());
    return E_FAIL;
  }

  // Show nearby command lists after execution
  ShowNearbyCommandListsInternal(client, "");
  return S_OK;
}

HRESULT CALLBACK ListCommandListsInternal(IDebugClient* client,
                                          const char* args) {
  if (args && args[0] == '?' && args[1] == '\0') {
    DOUT(
        "ListCommandLists - List all recorded command lists\n\n"
        "Usage: !ListCommandLists [options]\n\n"
        "Options:\n"
        "  (no args)       - List all command lists\n"
        "  text            - List command lists containing 'text' (case insensitive)\n"
        "  number          - Show detailed view of command list at index 'number'\n"
        "  .               - List all command lists in the current source file\n"
        "  s:text_pattern  - List command lists containing 'text_pattern' (case insensitive)\n"
        "  n:text_pattern  - List command lists with names containing 'text_pattern' (case insensitive)\n"
        "  .:number        - Show detailed view of command list at line 'number' in current file\n\n"
        "Examples:\n"
        "  !ListCommandLists              - List all command lists\n"
        "  !ListCommandLists test         - List command lists containing 'test'\n"
        "  !ListCommandLists 0            - Show detailed view of command list at index 0\n"
        "  !ListCommandLists .            - List all command lists in current source file\n"
        "  !ListCommandLists s:init       - List command lists containing 'init'\n"
        "  !ListCommandLists n:MyList     - List command lists with names containing 'MyList'\n"
        "  !ListCommandLists .:123        - Show detailed view of command list at line 123 in current file\n");
    return S_OK;
  }

  if (g_command_lists.empty()) {
    DOUT("No command lists found.\n");
    return S_OK;
  }

  std::vector<std::string> parsed_args = utils::ParseCommandLine(args);

  // Handle empty args
  if (parsed_args.empty()) {
    DOUT("Command Lists:\n");
    size_t index = 0;
    for (const auto& cmd_list : g_command_lists) {
      DOUT("  %u) %s\n", index, cmd_list.ToMediumString().c_str());
      index++;
    }
    DOUT("\n");
    return S_OK;
  }

  // Handle single argument
  if (parsed_args.size() == 1) {
    std::string arg = parsed_args[0];

    // Check if it's just "." - list all command lists in current file
    if (arg == ".") {
      utils::SourceInfo source_info = utils::GetCurrentSourceInfo(&g_debug);
      if (!source_info.is_valid) {
        DERROR("Could not get current source information.\n");
        return E_FAIL;
      }

      DOUT("Command Lists in %s:\n", source_info.file_name.c_str());
      size_t index = 0;
      bool found = false;
      for (const auto& cmd_list : g_command_lists) {
        if (cmd_list.GetSourceFile() == source_info.full_path) {
          DOUT("  %u) %s\n", index, cmd_list.ToMediumString().c_str());
          found = true;
        }
        index++;
      }
      if (!found) {
        DOUT("  No command lists found in current file.\n");
      }
      DOUT("\n");
      return S_OK;
    }

    // Check if it's a plain number (index)
    if (utils::IsWholeNumber(arg)) {
      size_t index = std::stoul(arg);
      if (index >= g_command_lists.size()) {
        DERROR("Index %u is out of range. Valid range: 0-%u\n", index,
               g_command_lists.size() - 1);
        return E_INVALIDARG;
      }

      DOUT("Command List [%u]:\n", index);
      DOUT("%s", g_command_lists[index].ToLongString("\t").c_str());
      DOUT("\n");
      return S_OK;
    }

    // Check if it has a colon (special option)
    size_t colon_pos = arg.find(':');
    if (colon_pos != std::string::npos) {
      if (colon_pos == 0 || colon_pos == arg.length() - 1) {
        DERROR("Invalid option format. Expected format: 'option:value'\n");
        return E_INVALIDARG;
      }

      std::string option_type = arg.substr(0, colon_pos);
      std::string option_value = arg.substr(colon_pos + 1);

      if (option_type == "s") {
        // Search by text pattern
        DOUT("Command Lists containing '%s':\n", option_value.c_str());
        size_t index = 0;
        bool found = false;
        for (const auto& cmd_list : g_command_lists) {
          if (cmd_list.HasTextMatch(option_value)) {
            DOUT("  %u) %s\n", index, cmd_list.ToMediumString().c_str());
            found = true;
          }
          index++;
        }
        if (!found) {
          DOUT("  No command lists found containing '%s'\n",
               option_value.c_str());
        }
        DOUT("\n");
      } else if (option_type == "n") {
        // Search by name pattern
        DOUT("Command Lists with names containing '%s':\n",
             option_value.c_str());
        size_t index = 0;
        bool found = false;
        for (const auto& cmd_list : g_command_lists) {
          if (cmd_list.HasNameMatch(option_value)) {
            DOUT("  %u) %s\n", index, cmd_list.ToMediumString().c_str());
            found = true;
          }
          index++;
        }
        if (!found) {
          DOUT("  No command lists found with names containing '%s'\n",
               option_value.c_str());
        }
        DOUT("\n");
      } else if (option_type == ".") {
        // Show detailed view by line number in current file
        if (!utils::IsWholeNumber(option_value)) {
          DERROR("Invalid line number. Expected a number.\n");
          return E_INVALIDARG;
        }

        int line = std::stoi(option_value);
        utils::SourceInfo source_info = utils::GetCurrentSourceInfo(&g_debug);
        if (!source_info.is_valid) {
          DERROR("Could not get current source information.\n");
          return E_FAIL;
        }

        // Find command list at specified line in current file
        bool found = false;
        size_t index = 0;
        for (const auto& cmd_list : g_command_lists) {
          if (cmd_list.GetSourceFile() == source_info.full_path &&
              cmd_list.GetSourceLine() == line) {
            DOUT("Command List at line %d in %s:\n", line,
                 source_info.file_name.c_str());
            DOUT("%s", cmd_list.ToLongString("\t").c_str());
            DOUT("\n");
            found = true;
            break;
          }
          index++;
        }

        if (!found) {
          DERROR("No command list found at line %d in current file.\n", line);
          return E_FAIL;
        }
      } else {
        DERROR("Unknown option: '%s'. Use '?' for help.\n",
               option_type.c_str());
        return E_INVALIDARG;
      }
    } else {
      // Plain text search
      DOUT("Command Lists:\n");
      size_t index = 0;
      for (const auto& cmd_list : g_command_lists) {
        if (cmd_list.HasTextMatch(arg)) {
          DOUT("  %u) %s\n", index, cmd_list.ToMediumString().c_str());
        }
        index++;
      }
      DOUT("\n");
    }

    return S_OK;
  }

  // Multiple arguments
  DERROR("Invalid arguments. Expected exactly one option.\n");
  return E_INVALIDARG;
}

HRESULT CALLBACK RemoveCommandListInternal(IDebugClient* client,
                                           const char* args) {
  if (args && args[0] == '?' && args[1] == '\0') {
    DOUT(
        "RemoveCommandList - Remove a command list from history\n\n"
        "Usage: !RemoveCommandList <option>\n\n"
        "Options:\n"
        "  number   - Remove command list at index 'number'\n"
        "  .:number - Remove command list at line 'number' in current file\n\n"
        "Examples:\n"
        "  !RemoveCommandList 0      - Remove command list at index 0\n"
        "  !RemoveCommandList .:123  - Remove command list at line 123 in current file\n");
    return S_OK;
  }

  if (g_command_lists.empty()) {
    DOUT("No command lists found.\n");
    return S_OK;
  }

  std::vector<std::string> parsed_args = utils::ParseCommandLine(args);

  if (parsed_args.empty()) {
    DERROR("No arguments provided. Use '?' for help.\n");
    return E_INVALIDARG;
  }

  if (parsed_args.size() > 1) {
    DERROR("Too many arguments. Expected exactly one option.\n");
    return E_INVALIDARG;
  }

  std::string arg = parsed_args[0];
  size_t index_to_remove = SIZE_MAX;

  // Check if it's a line number option (.:number)
  if (arg.length() > 2 && arg[0] == '.' && arg[1] == ':') {
    std::string line_str = arg.substr(2);

    if (!utils::IsWholeNumber(line_str)) {
      DERROR("Invalid line number. Expected a number after '.:'.\n");
      return E_INVALIDARG;
    }

    int line = std::stoi(line_str);
    utils::SourceInfo source_info = utils::GetCurrentSourceInfo(&g_debug);
    if (!source_info.is_valid) {
      DERROR("Could not get current source information.\n");
      return E_FAIL;
    }

    // Find command list at specified line in current file
    bool found = false;
    for (size_t i = 0; i < g_command_lists.size(); i++) {
      if (g_command_lists[i].GetSourceFile() == source_info.full_path &&
          g_command_lists[i].GetSourceLine() == line) {
        index_to_remove = i;
        found = true;
        break;
      }
    }

    if (!found) {
      DERROR("No command list found at line %d in current file.\n", line);
      return E_FAIL;
    }
  }
  // Check if it's a plain index number
  else if (utils::IsWholeNumber(arg)) {
    size_t index = std::stoul(arg);
    if (index >= g_command_lists.size()) {
      DERROR("Index %u is out of range. Valid range: 0-%u\n", index,
             g_command_lists.size() - 1);
      return E_INVALIDARG;
    }
    index_to_remove = index;
  } else {
    DERROR(
        "Invalid argument. Expected a number or '.:number'. Use '?' for help.\n");
    return E_INVALIDARG;
  }

  // Display what will be removed
  DOUT("\nRemoving command list:\n");
  DOUT("  [%u] %s\n\n", index_to_remove,
       g_command_lists[index_to_remove].ToMediumString().c_str());

  // Remove the command list
  g_command_lists.erase(g_command_lists.begin() + index_to_remove);
  WriteCommandListsToFile();

  DOUT("Command list removed successfully.\n");
  return S_OK;
}

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
  UnregisterBreakEventHandler();

  if (g_current_command_list) {
    delete g_current_command_list;
    g_current_command_list = nullptr;
  }

  return utils::UninitializeDebugInterfaces(&g_debug);
}

HRESULT CALLBACK ConvertCommandListToJavascriptInternal(IDebugClient* client,
                                                        const char* args) {
  if (args && args[0] == '?' && args[1] == '\0') {
    DOUT(
        "ConvertCommandListToJavascript - Convert a command list to a JavaScript function\n\n"
        "Usage: !ConvertCommandListToJavascript <index>\n\n"
        "  <index> - Index of the command list to convert\n\n"
        "Examples:\n"
        "  !ConvertCommandListToJavascript 0  - Convert command list at index 0 to JavaScript\n\n"
        "The generated JavaScript function will be named 'Run[CommandListName]Commands'\n"
        "where [CommandListName] is the name of the command list.\n");
    return S_OK;
  }

  if (g_command_lists.empty()) {
    DOUT("No command lists found.\n");
    return S_OK;
  }

  std::vector<std::string> parsed_args = utils::ParseCommandLine(args);

  if (parsed_args.empty()) {
    DERROR("No index provided. Use '?' for help.\n");
    return E_INVALIDARG;
  }

  if (parsed_args.size() > 1) {
    DERROR("Too many arguments. Expected exactly one index.\n");
    return E_INVALIDARG;
  }

  std::string index_str = parsed_args[0];

  if (!utils::IsWholeNumber(index_str)) {
    DERROR("Invalid index. Expected a number.\n");
    return E_INVALIDARG;
  }

  size_t index = std::stoul(index_str);
  if (index >= g_command_lists.size()) {
    DERROR("Index %u is out of range. Valid range: 0-%u\n", index,
           g_command_lists.size() - 1);
    return E_INVALIDARG;
  }

  const CommandList& cmd_list = g_command_lists[index];

  // Generate function name
  std::string function_name = "Run";
  if (!cmd_list.GetName().empty()) {
    function_name += cmd_list.GetName();
  } else {
    function_name += "CommandList" + std::to_string(index);
  }
  function_name += "Commands";

  // Generate JavaScript code
  DOUT("\"use strict\";\n\n");

  // Add description comment if available
  if (!cmd_list.GetDescription().empty()) {
    DOUT("// %s\n", cmd_list.GetDescription().c_str());
  }

  DOUT("// Generated from command list at %s:%d\n",
       cmd_list.GetSourceFile().c_str(), cmd_list.GetSourceLine());

  DOUT("function %s() {\n", function_name.c_str());
  DOUT("    let ctl = host.namespace.Debugger.Utility.Control;\n");

  std::string last_command;

  for (const auto& command : cmd_list.GetCommands()) {
    std::string command_to_execute;

    if (command.empty()) {
      // Use the previous command if current command is empty
      if (!last_command.empty()) {
        command_to_execute = last_command;
        DOUT("    // Repeat previous command\n");
      } else {
        continue;
      }
    } else {
      command_to_execute = command;
      last_command = command;
    }

    // Escape double quotes and backslashes in the command
    std::string escaped_command;
    for (char c : command_to_execute) {
      if (c == '"' || c == '\\') {
        escaped_command += '\\';
      }
      escaped_command += c;
    }

    DOUT("    ctl.ExecuteCommand(\"%s\");\n", escaped_command.c_str());
  }

  DOUT("}\n\n");
  DOUT("To use this function in WinDbg JavaScript:\n");
  DOUT("1. Save this code to a .js file\n");
  DOUT("2. Load it with: .scriptload <path_to_file>\n");
  DOUT("3. Execute with: dx @$scriptContents.%s()\n", function_name.c_str());
  return S_OK;
}

HRESULT CALLBACK SetCommandListAutoRunInternal(IDebugClient* client,
                                               const char* args) {
  if (args && args[0] == '?' && args[1] == '\0') {
    DOUT(
        "SetCommandListAutoRun - Set the auto-run flag for a command list\n\n"
        "Usage: !SetCommandListAutoRun <index> <value>\n\n"
        "Parameters:\n"
        "  <index> - Index of the command list to update\n"
        "  <value> - Auto-run setting: true, false, 1, or 0\n\n"
        "Examples:\n"
        "  !SetCommandListAutoRun 0 true   - Enable auto-run for command list at index 0\n"
        "  !SetCommandListAutoRun 2 false  - Disable auto-run for command list at index 2\n"
        "  !SetCommandListAutoRun 1 1      - Enable auto-run for command list at index 1\n"
        "  !SetCommandListAutoRun 3 0      - Disable auto-run for command list at index 3\n\n"
        "When auto-run is enabled, the command list will automatically execute when\n"
        "the debugger breaks at the exact source location where it was created.\n");
    return S_OK;
  }

  if (g_command_lists.empty()) {
    DOUT("No command lists found.\n");
    return S_OK;
  }

  std::vector<std::string> parsed_args = utils::ParseCommandLine(args);

  if (parsed_args.size() != 2) {
    DERROR(
        "Expected exactly 2 arguments: index and value. Use '?' for help.\n");
    return E_INVALIDARG;
  }

  // Parse the index
  std::string index_str = parsed_args[0];
  if (!utils::IsWholeNumber(index_str)) {
    DERROR("Invalid index. Expected a number.\n");
    return E_INVALIDARG;
  }

  size_t index = std::stoul(index_str);
  if (index >= g_command_lists.size()) {
    DERROR("Index %u is out of range. Valid range: 0-%u\n", index,
           g_command_lists.size() - 1);
    return E_INVALIDARG;
  }

  // Parse the boolean value
  std::string value_str = parsed_args[1];
  bool auto_run_value;

  // Convert to lowercase for case-insensitive comparison
  std::transform(value_str.begin(), value_str.end(), value_str.begin(),
                 ::tolower);

  if (value_str == "true" || value_str == "1") {
    auto_run_value = true;
  } else if (value_str == "false" || value_str == "0") {
    auto_run_value = false;
  } else {
    DERROR("Invalid value. Expected: true, false, 1, or 0\n");
    return E_INVALIDARG;
  }

  // Update the command list
  CommandList& cmd_list = g_command_lists[index];
  bool old_value = cmd_list.IsAutoRun();
  cmd_list.SetAutoRun(auto_run_value);

  // Save changes to file
  WriteCommandListsToFile();

  // Display confirmation
  DOUT("Command list [%u]: %s\n", index, cmd_list.ToMediumString().c_str());
  DOUT("Auto-run changed from %s to %s\n", old_value ? "enabled" : "disabled",
       auto_run_value ? "enabled" : "disabled");

  if (auto_run_value) {
    DOUT(
        "\nThis command list will now execute automatically when the debugger\n");
    DOUT("breaks at %s:%d with matching source context.\n",
         cmd_list.GetSourceFile().c_str(), cmd_list.GetSourceLine());
  }

  return S_OK;
}

HRESULT CALLBACK SetCommandListsFileInternal(IDebugClient* client,
                                             const char* args) {
  if (args && args[0] == '?' && args[1] == '\0') {
    DOUT(
        "SetCommandListsFile - Set the path for command lists file\n\n"
        "Usage: !SetCommandListsFile <filePath>\n\n"
        "Parameters:\n"
        "  <filePath> - The full path to the command lists JSON file\n"
        "               Must be a valid file path\n"
        "               The directory must exist (file will be created if needed)\n\n"
        "Examples:\n"
        "  !SetCommandListsFile C:\\Debugger\\my_commands.json            - Set custom command lists file\n"
        "  !SetCommandListsFile D:\\Projects\\debug\\command_lists.json   - Use project-specific commands\n\n"
        "Notes:\n"
        "- The path is not persisted between debugging sessions\n"
        "- If the file doesn't exist, it will be created when command lists are saved\n"
        "- If the file exists but is invalid, the command lists will be cleared\n"
        "- The default location is in the same directory as the extension DLL\n");
    return S_OK;
  }

  if (!args || strlen(args) == 0) {
    DERROR("No file path provided. Use '?' for help.\n");
    return E_INVALIDARG;
  }

  // Parse the file path argument
  std::vector<std::string> parsed_args = utils::ParseCommandLine(args);
  if (parsed_args.empty()) {
    DERROR("Invalid file path.\n");
    return E_INVALIDARG;
  }

  if (parsed_args.size() > 1) {
    DERROR("Too many arguments. Expected a single file path.\n");
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
      DERROR("Directory does not exist: %s\n", directory.c_str());
      return E_INVALIDARG;
    }
  }

  // Set the new file path
  g_command_lists_file = new_file_path;

  DOUT("Setting command lists file to: %s\n", g_command_lists_file.c_str());

  // Reinitialize command lists with the new file
  InitializeCommandLists();

  DOUT("Loaded %u command list(s) from the new file.\n",
       g_command_lists.size());
  return S_OK;
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
ShowNearbyCommandLists(IDebugClient* client, const char* args) {
  return ShowNearbyCommandListsInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK
StartCommandListRecording(IDebugClient* client, const char* args) {
  return StartCommandListRecordingInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK
StopCommandListRecording(IDebugClient* client, const char* args) {
  return StopCommandListRecordingInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK
PauseCommandListRecording(IDebugClient* client, const char* args) {
  return PauseCommandListRecordingInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK
ResumeCommandListRecording(IDebugClient* client, const char* args) {
  return ResumeCommandListRecordingInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK RunCommandList(IDebugClient* client,
                                                      const char* args) {
  return RunCommandListInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK ListCommandLists(IDebugClient* client,
                                                        const char* args) {
  return ListCommandListsInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK RemoveCommandList(IDebugClient* client,
                                                         const char* args) {
  return RemoveCommandListInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK
ConvertCommandListToJavascript(IDebugClient* client, const char* args) {
  return ConvertCommandListToJavascriptInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK
SetCommandListAutoRun(IDebugClient* client, const char* args) {
  return SetCommandListAutoRunInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK SetCommandListsFile(IDebugClient* client,
                                                           const char* args) {
  return SetCommandListsFileInternal(client, args);
}
}

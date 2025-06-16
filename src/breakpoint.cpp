// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#include "breakpoint.h"

#include <regex>
#include "utils.h"

Breakpoint::Breakpoint(const std::string& breakpoint_string) {
  ParseBreakpointString(breakpoint_string);
}

Breakpoint::Breakpoint(const std::string& location,
                       const std::string& module_name) {
  ParseBreakpointString(location);
  SetModuleName(module_name);
}

void Breakpoint::SetModuleName(const std::string& module_name) {
  module_name_ = utils::Trim(module_name);
}

std::string Breakpoint::GetLocation() const {
  if (is_source_line_) {
    if (line_number_ <= 0) {
      return "";
    }
    return location_ + ":" + std::to_string(line_number_);
  }
  return location_;
}

std::string Breakpoint::GetFullString() const {
  if (location_.empty() || module_name_.empty()) {
    return "";
  }

  if (is_source_line_) {
    if (line_number_ <= 0) {
      return "";
    }
    return "`" + module_name_ + "!" + location_ + ":" +
           std::to_string(line_number_) + "`";
  }

  return module_name_ + "!" + location_;
}

bool Breakpoint::IsValid() const {
  if (location_.empty() || module_name_.empty()) {
    return false;
  }

  if (is_source_line_ && line_number_ <= 0) {
    return false;
  }

  if (!is_source_line_ && line_number_ > 0) {
    return false;
  }

  return true;
}

std::string Breakpoint::GetPath() const {
  if (!is_source_line_) {
    return "";
  }
  return location_;
}

bool Breakpoint::UpdateLineNumber(int new_line_number) {
  if (!is_source_line_ || new_line_number <= 0) {
    return false;
  }

  line_number_ = new_line_number;
  return true;
}

JSON Breakpoint::ToJson() const {
  std::string location = location_;
  if (is_source_line_) {
    location += ":" + std::to_string(line_number_);
  }
  return JSON{{"location", location}, {"module_name", module_name_}};
}

Breakpoint Breakpoint::FromJson(const JSON& json) {
  // Check if required fields exist
  if (!json.contains("location") || !json.contains("module_name")) {
    return Breakpoint();
  }

  // Safely extract values with type checking
  std::string location = json["location"].get<std::string>();
  std::string module_name = json["module_name"].get<std::string>();
  return Breakpoint(location, module_name);
}

void Breakpoint::ParseBreakpointString(const std::string& input) {
  std::string bp = utils::Trim(input);

  if (bp.empty()) {
    return;
  }

  // Regex to match source:line breakpoints
  // Matches patterns like:
  // - `c:\path\file.cpp:123`
  // - c:\path\file.cpp:123
  // - c:/path/file.cpp:123
  // - `c:\\path\\file.cpp:123`
  // - `module!c:\path\file.cpp:123`
  // - ../test.cpp:123
  // - \\share\test.cpp:123
  std::regex source_line_regex(R"(^`?(?:([^!]+)!)?(.+?):(\d+)`?$)");
  std::smatch matches;

  if (std::regex_match(bp, matches, source_line_regex)) {
    // This is a source:line breakpoint
    // Capture group 1: optional module
    std::string module_name = matches[1].str();
    // Capture group 2: file path
    std::string file_path = matches[2].str();
    // Capture group 3: line number
    std::string line_number = matches[3].str();

    // Convert the file path
    std::string converted_path = utils::ConvertToBreakpointFilePath(file_path);

    if (converted_path.empty() || module_name.find(' ') != std::string::npos) {
      // Invalid file path or module name
      // contains invalid characters.
      return;
    }

    is_source_line_ = true;
    line_number_ = std::stoi(line_number);
    location_ = converted_path;
    module_name_ = utils::Trim(module_name);
  } else {
    // This is a standard breakpoint
    // Check if it contains a module separator
    size_t separator_pos = bp.find('!');

    if (separator_pos != std::string::npos) {
      module_name_ = utils::Trim(bp.substr(0, separator_pos));
      location_ = utils::Trim(bp.substr(separator_pos + 1));
    } else {
      // No module name, just the location
      location_ = utils::Trim(bp);
      module_name_ = "";
    }

    is_source_line_ = false;
    line_number_ = 0;
  }
}

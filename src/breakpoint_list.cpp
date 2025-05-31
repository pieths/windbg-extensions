// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#include "breakpoint_list.h"

#include <regex>
#include <set>
#include <sstream>

#include "utils.h"

BreakpointList::BreakpointList(const std::string& delimited_breakpoints,
                               const std::string& module_name,
                               const std::string& tag) {
  SetBreakpointsFromDelimitedString(delimited_breakpoints);
  SetModuleName(module_name);
  SetTag(tag);
}

void BreakpointList::SetBreakpointsFromDelimitedString(
    const std::string& input) {
  breakpoints_.clear();
  std::stringstream ss(input);
  std::string bp;

  while (std::getline(ss, bp, ';')) {
    bp = utils::Trim(bp);
    if (!bp.empty()) {
      breakpoints_.push_back(bp);
    }
  }
}

void BreakpointList::SetBreakpointsFromArray(
    const std::vector<std::string>& breakpoints) {
  breakpoints_.clear();
  for (const auto& bp : breakpoints) {
    std::string trimmed = utils::Trim(bp);
    if (!trimmed.empty()) {
      breakpoints_.push_back(trimmed);
    }
  }
}

void BreakpointList::SetModuleName(const std::string& module_name,
                                   bool update_breakpoints) {
  if (update_breakpoints) {
    // If a breakpoint has a module name that matches
    // the old module name, update it to the new module name.
    std::string old_module_name = utils::RemoveFileExtension(module_name_);
    std::string new_module_name = utils::RemoveFileExtension(module_name);

    // Update breakpoints that match the old module name
    for (auto& bp : breakpoints_) {
      size_t bangPos = bp.find('!');
      if (bangPos != std::string::npos) {
        std::string bp_module_name = bp.substr(0, bangPos);
        if (bp_module_name == old_module_name) {
          bp = new_module_name + "!" + bp.substr(bangPos + 1);
        }
      }
    }

    // Remove duplicates
    std::set<std::string> uniqueBreakpoints(breakpoints_.begin(),
                                            breakpoints_.end());
    breakpoints_.assign(uniqueBreakpoints.begin(), uniqueBreakpoints.end());
  }

  module_name_ = module_name;
}

std::string BreakpointList::GetBreakpointAtIndex(size_t index) const {
  return (index >= breakpoints_.size()) ? "" : breakpoints_[index];
}

bool BreakpointList::IsValid() const {
  return !breakpoints_.empty() && !module_name_.empty();
}

bool BreakpointList::HasTextMatch(const std::string& search_term) const {
  if (utils::ContainsCI(module_name_, search_term) ||
      utils::ContainsCI(tag_, search_term)) {
    return true;
  }

  for (const auto& bp : breakpoints_) {
    if (utils::ContainsCI(bp, search_term)) {
      return true;
    }
  }

  return false;
}

bool BreakpointList::HasTagMatch(const std::string& search_term) const {
  return utils::ContainsCI(tag_, search_term);
}

bool BreakpointList::IsEqualTo(const BreakpointList& other) const {
  if (module_name_ != other.module_name_ || tag_ != other.tag_ ||
      breakpoints_.size() != other.breakpoints_.size()) {
    return false;
  }

  std::set<std::string> this_set(breakpoints_.begin(), breakpoints_.end());
  std::set<std::string> other_set(other.breakpoints_.begin(),
                                  other.breakpoints_.end());
  return this_set == other_set;
}

std::string BreakpointList::GetCombinedCommandString() const {
  std::string command_string;
  for (const auto& bp : breakpoints_) {
    command_string += "bp " + bp + "; ";
  }
  return command_string;
}

bool BreakpointList::UpdateLineNumber(size_t index, int new_line_number) {
  if (index >= breakpoints_.size()) {
    return false;
  }

  std::string& bp = breakpoints_[index];

  // Match pattern like ":123`" where 123 is any number
  std::regex line_pattern(":([0-9]+)(`)");
  std::smatch matches;

  if (std::regex_search(bp, matches, line_pattern) && matches.size() >= 3) {
    // Construct new string with updated line number
    std::string before = bp.substr(0, matches.position(1));
    std::string after = bp.substr(matches.position(1) + matches.length(1));

    // Replace the line number but keep the backtick
    bp = before + std::to_string(new_line_number) + after;
    return true;
  }

  // Could not find a line number pattern in this breakpoint
  return false;
}

bool BreakpointList::UpdateFirstLineNumber(int new_line_number) {
  // Update the first breakpoints line number where UpdateLineNumber returns
  // true
  for (size_t i = 0; i < breakpoints_.size(); ++i) {
    if (UpdateLineNumber(i, new_line_number)) {
      // Successfully updated the first found source:line breakpoint.
      return true;
    }
  }
  return false;  // No breakpoints were updated
}

std::string BreakpointList::GetSxeString() const {
  // Escape all quotes in the command string
  std::string command_string = GetCombinedCommandString();
  std::string escaped_command = utils::EscapeQuotes(command_string);

  std::string sxe_string = "sxe -c \"" + escaped_command + " gc\" ";
  if (module_name_.find(".exe") != std::string::npos) {
    sxe_string += "cpr:";
  } else {
    sxe_string += "ld:";
  }
  sxe_string += module_name_;
  return sxe_string;
}

std::string BreakpointList::ToShortString() const {
  std::string outputString = module_name_;
  if (!tag_.empty()) {
    outputString += " (" + tag_ + ")";
  }
  outputString += " [";

  for (size_t i = 0; i < breakpoints_.size(); i++) {
    if (i > 0) {
      outputString += "; ";
    }
    outputString += breakpoints_[i];
  }

  return outputString + "]";
}

std::string BreakpointList::ToLongString(const std::string& indent) const {
  std::string output_string;
  if (!tag_.empty()) {
    output_string += indent + "TAG:    " + tag_ + "\n";
  }
  output_string += indent + "MODULE: " + module_name_ + "\n\n";

  for (const auto& bp : breakpoints_) {
    output_string += indent + bp + "\n";
  }
  return output_string;
}

JSON BreakpointList::ToJson() const {
  JSON json;
  json["tag"] = tag_;
  json["moduleName"] = module_name_;
  json["breakpoints"] = breakpoints_;
  return json;
}

BreakpointList BreakpointList::FromJson(const JSON& json) {
  BreakpointList breakpoint_list;

  if (json.contains("tag") && json["tag"].is_string()) {
    breakpoint_list.tag_ = json["tag"].get<std::string>();
  }

  if (json.contains("moduleName") && json["moduleName"].is_string()) {
    breakpoint_list.module_name_ = json["moduleName"].get<std::string>();
  }

  if (json.contains("breakpoints") && json["breakpoints"].is_array()) {
    for (const auto& bp : json["breakpoints"]) {
      if (bp.is_string()) {
        breakpoint_list.breakpoints_.push_back(bp.get<std::string>());
      }
    }
  }

  return breakpoint_list;
}

BreakpointList BreakpointList::CombineBreakpointLists(
    const BreakpointList& list1,
    const BreakpointList& list2) {
  BreakpointList combined;
  combined.module_name_ = list1.module_name_;
  combined.tag_ = list1.tag_;

  // Combine and deduplicate breakpoints
  std::set<std::string> uniqueBreakpoints;
  for (const auto& bp : list1.breakpoints_) {
    uniqueBreakpoints.insert(bp);
  }
  for (const auto& bp : list2.breakpoints_) {
    uniqueBreakpoints.insert(bp);
  }

  combined.breakpoints_.assign(uniqueBreakpoints.begin(),
                               uniqueBreakpoints.end());
  return combined;
}

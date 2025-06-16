// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#include "breakpoint_list.h"

#include <set>
#include <sstream>

#include "utils.h"

BreakpointList::BreakpointList(const std::string& delimited_breakpoints,
                               const std::string& default_module_name,
                               const std::string& tag) {
  SetBreakpointsFromDelimitedString(delimited_breakpoints, default_module_name);
  SetTag(tag);
}

void BreakpointList::SetBreakpointsFromDelimitedString(
    const std::string& input,
    const std::string& default_module_name) {
  std::vector<std::string> breakpoint_strings;
  std::stringstream ss(input);
  std::string bp_string;

  while (std::getline(ss, bp_string, ',')) {
    bp_string = utils::Trim(bp_string);
    if (!bp_string.empty()) {
      breakpoint_strings.push_back(bp_string);
    }
  }

  SetBreakpointsFromArray(breakpoint_strings, default_module_name);
}

void BreakpointList::SetBreakpointsFromArray(
    const std::vector<std::string>& breakpoints,
    const std::string& default_module_name) {
  breakpoints_.clear();
  for (const auto& bp_string : breakpoints) {
    std::string trimmed = utils::Trim(bp_string);
    if (!trimmed.empty()) {
      Breakpoint bp(trimmed);

      // If the breakpoint doesn't have a module name and
      // we have a default use the default module name
      if (bp.GetModuleName().empty() && !default_module_name.empty()) {
        bp.SetModuleName(default_module_name);
      }

      if (!bp.IsValid()) {
        // Any invalid breakpoint should
        // invalidate the entire list
        breakpoints_.clear();
        return;
      }

      breakpoints_.push_back(bp);
    }
  }
}

void BreakpointList::AddBreakpoint(const Breakpoint& breakpoint) {
  if (breakpoint.IsValid()) {
    // Check if this breakpoint already exists in the list
    for (const auto& existing_bp : breakpoints_) {
      if (existing_bp.GetFullString() == breakpoint.GetFullString()) {
        return;  // Breakpoint already exists, don't add it
      }
    }
    breakpoints_.push_back(breakpoint);
  }
}

void BreakpointList::ReplaceAllModuleNames(const std::string& new_module_name) {
  for (auto& bp : breakpoints_) {
    bp.SetModuleName(new_module_name);
  }
}

Breakpoint BreakpointList::GetBreakpointAtIndex(size_t index) const {
  if (index >= breakpoints_.size()) {
    return Breakpoint();  // Return invalid breakpoint
  }
  return breakpoints_[index];
}

bool BreakpointList::IsValid() const {
  if (breakpoints_.empty()) {
    return false;
  }

  // All breakpoints must be valid
  for (const auto& bp : breakpoints_) {
    if (!bp.IsValid()) {
      return false;
    }
  }

  return true;
}

bool BreakpointList::HasTextMatch(const std::string& search_term) const {
  if (utils::ContainsCI(tag_, search_term)) {
    return true;
  }

  for (const auto& bp : breakpoints_) {
    if (utils::ContainsCI(bp.GetFullString(), search_term)) {
      return true;
    }
  }

  return false;
}

bool BreakpointList::HasTagMatch(const std::string& search_term) const {
  return utils::ContainsCI(tag_, search_term);
}

bool BreakpointList::IsEqualTo(const BreakpointList& other) const {
  if (tag_ != other.tag_ || breakpoints_.size() != other.breakpoints_.size()) {
    return false;
  }

  // Create sets of full breakpoint strings for comparison
  std::set<std::string> this_set;
  std::set<std::string> other_set;

  for (const auto& bp : breakpoints_) {
    this_set.insert(bp.GetFullString());
  }

  for (const auto& bp : other.breakpoints_) {
    other_set.insert(bp.GetFullString());
  }

  return this_set == other_set;
}

std::string BreakpointList::GetCombinedCommandString() const {
  std::string command_string;
  for (const auto& bp : breakpoints_) {
    command_string += "bp " + bp.GetFullString() + "; ";
  }
  return command_string;
}

bool BreakpointList::UpdateLineNumber(size_t index, int new_line_number) {
  if (index >= breakpoints_.size()) {
    return false;
  }

  Breakpoint& bp = breakpoints_[index];
  if (!bp.IsSourceLineBreakpoint()) {
    return false;
  }

  return bp.UpdateLineNumber(new_line_number);
}

bool BreakpointList::UpdateFirstLineNumber(int new_line_number) {
  if (new_line_number <= 0) {
    return false;  // Invalid line number
  }

  for (size_t i = 0; i < breakpoints_.size(); ++i) {
    if (UpdateLineNumber(i, new_line_number)) {
      return true;
    }
  }
  return false;
}

std::string BreakpointList::GetSxeString(const std::string& module_name) const {
  // Escape all quotes in the command string
  std::string command_string = GetCombinedCommandString();
  std::string escaped_command = utils::EscapeQuotes(command_string);

  std::string sxe_string = "sxe -c \"" + escaped_command + " gc\" ";
  if (module_name.find(".exe") != std::string::npos) {
    sxe_string += "cpr:";
  } else {
    sxe_string += "ld:";
  }
  sxe_string += module_name;
  return sxe_string;
}

std::string BreakpointList::ToShortString() const {
  std::string outputString;

  if (!tag_.empty()) {
    outputString += " (" + tag_ + ") ";
  }
  outputString += "[";

  for (size_t i = 0; i < breakpoints_.size(); i++) {
    if (i > 0) {
      outputString += "; ";
    }
    outputString += breakpoints_[i].GetFullString();
  }

  return outputString + "]";
}

std::string BreakpointList::ToLongString(const std::string& indent) const {
  std::string output_string;
  if (!tag_.empty()) {
    output_string += indent + "TAG:    " + tag_ + "\n";
  }

  // Group breakpoints by module
  std::map<std::string, std::vector<const Breakpoint*>> module_breakpoints;
  for (const auto& bp : breakpoints_) {
    module_breakpoints[bp.GetModuleName()].push_back(&bp);
  }

  for (const auto& [module, bps] : module_breakpoints) {
    output_string += indent + "MODULE: " + module + "\n";
    for (const auto* bp : bps) {
      output_string += indent + "  " + bp->GetLocation() + "\n";
    }
    output_string += indent + "\n";
  }

  return output_string;
}

JSON BreakpointList::ToJson() const {
  JSON json;
  json["tag"] = tag_;

  JSON breakpoints_json = JSON::array();
  for (const auto& bp : breakpoints_) {
    breakpoints_json.push_back(bp.ToJson());
  }
  json["breakpoints"] = breakpoints_json;
  return json;
}

BreakpointList BreakpointList::FromJson(const JSON& json) {
  BreakpointList breakpoint_list;

  if (json.contains("tag") && json["tag"].is_string()) {
    breakpoint_list.tag_ = json["tag"].get<std::string>();
  }

  if (json.contains("breakpoints") && json["breakpoints"].is_array()) {
    for (const auto& bp_json : json["breakpoints"]) {
      Breakpoint bp = Breakpoint::FromJson(bp_json);
      if (bp.IsValid()) {
        breakpoint_list.breakpoints_.push_back(bp);
      }
    }
  }

  return breakpoint_list;
}

BreakpointList BreakpointList::CombineBreakpointLists(
    const BreakpointList& list1,
    const BreakpointList& list2) {
  BreakpointList combined;
  combined.tag_ = list1.tag_;

  // Combine and deduplicate breakpoints based on their full string
  // representation
  std::set<std::string> unique_breakpoints;
  std::vector<Breakpoint> combined_breakpoints;

  for (const auto& bp : list1.breakpoints_) {
    std::string full_string = bp.GetFullString();
    if (unique_breakpoints.insert(full_string).second) {
      combined_breakpoints.push_back(bp);
    }
  }

  for (const auto& bp : list2.breakpoints_) {
    std::string full_string = bp.GetFullString();
    if (unique_breakpoints.insert(full_string).second) {
      combined_breakpoints.push_back(bp);
    }
  }

  combined.breakpoints_ = std::move(combined_breakpoints);
  return combined;
}

// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#include "command_list.h"

#include "utils.h"

CommandList::CommandList(std::vector<std::string> commands,
                         int source_line,
                         const std::string& source_file,
                         const std::string& name,
                         const std::string& description,
                         const std::string& source_context)
    : commands_(commands),
      source_line_(source_line),
      source_file_(source_file),
      name_(name),
      description_(description),
      source_context_(source_context) {}

void CommandList::SetCommands(const std::vector<std::string>& commands) {
  commands_.clear();
  for (const auto& cmd : commands) {
    std::string trimmed = utils::Trim(cmd);
    commands_.push_back(trimmed);
  }
}

bool CommandList::IsValid() const {
  return !commands_.empty() && !source_file_.empty();
}

bool CommandList::HasTextMatch(const std::string& search_term) const {
  if (utils::ContainsCI(source_file_, search_term) ||
      utils::ContainsCI(name_, search_term) ||
      utils::ContainsCI(description_, search_term)) {
    return true;
  }

  for (const auto& cmd : commands_) {
    if (utils::ContainsCI(cmd, search_term)) {
      return true;
    }
  }

  return false;
}

bool CommandList::HasNameMatch(const std::string& search_term) const {
  return utils::ContainsCI(name_, search_term);
}

bool CommandList::IsEqualTo(const CommandList& other) const {
  if (source_line_ != other.source_line_ ||
      source_file_ != other.source_file_ || name_ != other.name_ ||
      description_ != other.description_ ||
      commands_.size() != other.commands_.size()) {
    return false;
  }

  // Commands order matters for command lists
  for (size_t i = 0; i < commands_.size(); i++) {
    if (commands_[i] != other.commands_[i]) {
      return false;
    }
  }

  return true;
}

std::string CommandList::ToShortString(int index) const {
  std::string output_string = "(";
  if (index >= 0) {
    output_string += std::to_string(index) + ":";
  }
  output_string += std::to_string(source_line_);
  if (!name_.empty()) {
    output_string += ":" + name_;
  }
  if (auto_run_) {
    output_string += ":AUTO_RUN";
  }
  output_string += ")";
  return output_string;
}

std::string CommandList::ToMediumString() const {
  std::string output_string = ToShortString();
  if (!description_.empty()) {
    output_string += " - " + description_;
  }
  return output_string;
}

std::string CommandList::ToLongString(const std::string& indent) const {
  std::string output_string;

  if (!name_.empty()) {
    output_string += indent + "NAME:   " + name_ + "\n";
  }
  if (!description_.empty()) {
    output_string += indent + "DESC:   " + description_ + "\n";
  }
  output_string += indent + "LINE:   " + std::to_string(source_line_) + "\n";
  output_string += indent + "FILE:   " + source_file_ + "\n\n";
  output_string += indent + "AUTO RUN: " + (auto_run_ ? "YES" : "NO") + "\n";
  output_string += indent + "COMMANDS:\n";

  for (const auto& cmd : commands_) {
    output_string += indent + "\t" + cmd + "\n";
  }

  if (!source_context_.empty()) {
    output_string += "\n" + indent + "SOURCE CONTEXT:\n";
    std::vector<std::string> context_lines =
        utils::SplitString(source_context_, "\n", false);
    for (const auto& line : context_lines) {
      output_string += indent + "\t" + line + "\n";
    }
  }

  return output_string;
}

JSON CommandList::ToJson() const {
  JSON json;
  json["sourceLine"] = source_line_;
  json["sourceFile"] = source_file_;
  json["name"] = name_;
  json["description"] = description_;
  json["commands"] = commands_;
  json["sourceContext"] = source_context_;
  json["autoRun"] = auto_run_;
  return json;
}

CommandList CommandList::FromJson(const JSON& json) {
  CommandList command_list;

  if (json.contains("sourceLine") && json["sourceLine"].is_number()) {
    command_list.source_line_ = json["sourceLine"].get<int>();
  }

  if (json.contains("sourceFile") && json["sourceFile"].is_string()) {
    command_list.source_file_ = json["sourceFile"].get<std::string>();
  }

  if (json.contains("name") && json["name"].is_string()) {
    command_list.name_ = json["name"].get<std::string>();
  }

  if (json.contains("description") && json["description"].is_string()) {
    command_list.description_ = json["description"].get<std::string>();
  }

  if (json.contains("sourceContext") && json["sourceContext"].is_string()) {
    command_list.source_context_ = json["sourceContext"].get<std::string>();
  }

  if (json.contains("autoRun") && json["autoRun"].is_boolean()) {
    command_list.auto_run_ = json["autoRun"].get<bool>();
  }

  if (json.contains("commands") && json["commands"].is_array()) {
    for (const auto& cmd : json["commands"]) {
      if (cmd.is_string()) {
        command_list.commands_.push_back(cmd.get<std::string>());
      }
    }
  }

  return command_list;
}

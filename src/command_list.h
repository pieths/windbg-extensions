// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#ifndef COMMAND_LIST_H_
#define COMMAND_LIST_H_

#include <string>
#include <vector>

#include "json.hpp"

using JSON = nlohmann::json;

class CommandList {
 public:
  CommandList() = default;
  explicit CommandList(std::vector<std::string> commands,
                       int source_line = -1,
                       const std::string& source_file = "",
                       const std::string& name = "",
                       const std::string& description = "",
                       const std::string& source_context = "");

  void SetCommands(const std::vector<std::string>& commands);
  void SetSourceLine(int source_line) { source_line_ = source_line; }
  void SetSourceFile(const std::string& source_file) {
    source_file_ = source_file;
  }
  void SetName(const std::string& name) { name_ = name; }
  void SetDescription(const std::string& description) {
    description_ = description;
  }
  void SetSourceContext(const std::string& context) {
    source_context_ = context;
  }
  void SetAutoRun(bool auto_run) { auto_run_ = auto_run; }

  const std::vector<std::string>& GetCommands() const { return commands_; }

  int GetSourceLine() const { return source_line_; }
  std::string GetSourceFile() const { return source_file_; }
  std::string GetName() const { return name_; }
  std::string GetDescription() const { return description_; }
  std::string GetSourceContext() const { return source_context_; }
  bool IsAutoRun() const { return auto_run_; }

  size_t GetCommandsCount() const { return commands_.size(); }

  bool HasTextMatch(const std::string& search_term) const;
  bool HasNameMatch(const std::string& search_term) const;

  bool IsValid() const;
  bool IsEqualTo(const CommandList& other) const;

  // If index is -1 then do not include index in the output
  std::string ToShortString(int index = -1) const;
  std::string ToMediumString() const;
  std::string ToLongString(const std::string& indent = "") const;

  JSON ToJson() const;
  static CommandList FromJson(const JSON& json);

 private:
  std::vector<std::string> commands_;
  int source_line_ = 0;
  std::string source_file_;
  std::string name_;
  std::string description_;
  std::string source_context_;
  bool auto_run_ = false;
};

#endif  // COMMAND_LIST_H_

// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#ifndef BREAKPOINT_LIST_H_
#define BREAKPOINT_LIST_H_

#include <string>
#include <vector>

#include "json.hpp"

using JSON = nlohmann::json;

class BreakpointList {
 public:
  BreakpointList() = default;
  explicit BreakpointList(const std::string& delimited_breakpoints,
                          const std::string& module_name = "",
                          const std::string& tag = "");

  void SetBreakpointsFromDelimitedString(const std::string& input);
  void SetBreakpointsFromArray(const std::vector<std::string>& breakpoints);
  void SetModuleName(const std::string& new_module_name,
                     bool update_breakpoints = false);
  void SetTag(const std::string& tag) { tag_ = tag; }

  std::string GetBreakpointAtIndex(size_t index) const;
  std::string GetModuleName() const { return module_name_; }
  std::string GetTag() const { return tag_; }

  size_t GetBreakpointsCount() const { return breakpoints_.size(); }

  const std::vector<std::string>& GetBreakpoints() const {
    return breakpoints_;
  }

  bool HasTextMatch(const std::string& search_term) const;
  bool HasTagMatch(const std::string& search_term) const;

  bool IsValid() const;
  bool IsEqualTo(const BreakpointList& other) const;

  bool UpdateLineNumber(size_t index, int new_line_number);
  bool UpdateFirstLineNumber(int new_line_number);

  std::string GetCombinedCommandString() const;
  std::string GetSxeString() const;

  std::string ToShortString() const;
  std::string ToLongString(const std::string& indent = "") const;

  JSON ToJson() const;
  static BreakpointList FromJson(const JSON& json);

  static BreakpointList CombineBreakpointLists(const BreakpointList& list1,
                                               const BreakpointList& list2);

 private:
  std::vector<std::string> breakpoints_;
  std::string module_name_;
  std::string tag_;
};

#endif  // BREAKPOINT_LIST_H_

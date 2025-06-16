// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#ifndef BREAKPOINT_H_
#define BREAKPOINT_H_

#include <string>
#include "json.hpp"

using JSON = nlohmann::json;

class Breakpoint {
 public:
  Breakpoint() = default;
  explicit Breakpoint(const std::string& breakpoint_string);
  explicit Breakpoint(const std::string& location,
                      const std::string& module_name);

  void SetModuleName(const std::string& module_name);

  // If this is a source line breakpoint then the
  // location is the file path plus the line number.
  // If this is a standard breakpoint then the location
  // is the function name plus optional offset or address.
  // Effectively this equates to everything after the "!".
  std::string GetLocation() const;

  // A valid breakpoint will always have a module name.
  std::string GetModuleName() const { return module_name_; }

  // Get the full breakpoint string with module name
  // prepended. This string can be used to set breakpoints.
  std::string GetFullString() const;

  bool IsValid() const;

  bool IsSourceLineBreakpoint() const { return is_source_line_; }
  int GetLineNumber() const { return line_number_; }

  // If this is a source line breakpoint then the
  // path is the file path without the line number.
  // If this is a standard breakpoint then this
  // returns an empty string.
  std::string GetPath() const;

  // Update the line number for source line breakpoints.
  // Returns true if successful, false if not a source
  // line breakpoint
  bool UpdateLineNumber(int new_line_number);

  JSON ToJson() const;
  static Breakpoint FromJson(const JSON& json);

  bool operator==(const Breakpoint& other) const {
    return is_source_line_ == other.is_source_line_ &&
           line_number_ == other.line_number_ && location_ == other.location_ &&
           module_name_ == other.module_name_;
  }

  bool operator!=(const Breakpoint& other) const { return !(*this == other); }

 private:
  void ParseBreakpointString(const std::string& input);

  bool is_source_line_ = false;
  int line_number_ = 0;
  std::string location_;
  std::string module_name_;
};

#endif  // BREAKPOINT_H_

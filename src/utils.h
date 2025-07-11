// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#ifndef UTILS_H_
#define UTILS_H_

#include <dbgeng.h>
#include <windows.h>
#include <optional>
#include <string>
#include <vector>

// Macros for debug output to make code less verbose
#define DOUT(format, ...) \
  g_debug.control->Output(DEBUG_OUTPUT_NORMAL, format, ##__VA_ARGS__)
#define DERROR(format, ...) \
  g_debug.control->Output(DEBUG_OUTPUT_ERROR, format, ##__VA_ARGS__)

namespace utils {

// Remove leading and trailing whitespace from a string
std::string Trim(const std::string& str);

// Case-insensitive string comparison
bool ContainsCI(const std::string& string, const std::string& substr);

// Check if a string represents a whole number.
bool IsWholeNumber(const std::string& input_str);

// Remove the file extension from a filename.
std::string RemoveFileExtension(const std::string& filename);

// Escape quotes in a string by replacing " with \"
std::string EscapeQuotes(const std::string& input);

// Get the directory of the current extension dll.
std::string GetCurrentExtensionDir();

struct DebugInterfaces {
  IDebugClient* client = nullptr;
  IDebugControl* control = nullptr;
  IDebugSymbols* symbols = nullptr;
  IDebugDataSpaces4* data_spaces = nullptr;
  IDebugSystemObjects* system_objects = nullptr;
  IDebugRegisters* registers = nullptr;
};

HRESULT InitializeDebugInterfaces(DebugInterfaces* interfaces);
HRESULT UninitializeDebugInterfaces(DebugInterfaces* interfaces);

struct SourceInfo {
  ULONG line = 0;
  std::string file_path;
  std::string file_name;
  std::string full_path;
  std::string source_context;
  bool is_valid = false;
};

// Get current source file and line information from the debugger
SourceInfo GetCurrentSourceInfo(const DebugInterfaces* interfaces);

// Splits a string into tokens based on a delimiter.
// If combine_consecutive_delimiters is true,
// consecutive delimiters are treated as a single delimiter.
// For example, "a,,b" with delimiter "," will yield ["a", "b"] if true,
// and ["a", "", "b"] if false.
std::vector<std::string> SplitString(
    const std::string& input,
    const std::string& delimiter,
    bool combine_consecutive_delimiters = true);

// Parses a string containing indices and returns a vector of size_t values.
// Supports multiple formats:
//  - Individual numbers: "1", "42"
//  - Space-separated numbers: "1 2 3"
//  - Ranges with hyphens: "1-5" (expands to 1,2,3,4,5)
//  - Mixed formats: "1 3-5 7" (expands to 1,3,4,5,7)
// Handles both ascending (1-5) and descending (5-1) ranges.
// Returns a vector with all parsed indices, sorted and with duplicates removed.
std::vector<size_t> GetIndicesFromString(const std::string& input_str);

// Parses a string containing numbers or dotted pairs (e.g., "1", "2.3").
// Returns true if the input is a valid number or dotted pair.
// If a dotted pair is found, first_number will contain the first part,
// and second_number will contain the second part as an optional size_t.
// If the input is a single number, second_number will be std::nullopt.
bool ParseNumberOrDottedPair(const std::string& input,
                             size_t& first_number,
                             std::optional<size_t>& second_number);

// Parse command-line arguments. Due to the way that WinDbg
// parses command arguments this implementation uses single
// quotes to delimit text with spaces that should be considered
// a single argument. For example:
//   !myext.cmd 'arg1 arg2' arg3
// will yield ["arg1 arg2", "arg3"].
// Double quotes could not be used because if a double quote
// is present in the command line as the first non-whitespace
// character, WinDbg will not pass it to the extension.
// Note: Backslashes are only used for escaping when they
// are immediately followed by a single quote. A backslash
// not before a single quote will be treated as a regular character.
std::vector<std::string> ParseCommandLine(const char* cmdLine);

// Execute a command and capture its output.
// Note, this function is a little bit of a hack and should only be used
// when there is no other direct way to achieve the desired result using
// the existing debug interfaces.
// Also note, this function sets the output callbacks temporarily to
// capture the output, so it can interfere with existing output callbacks
// if they are set by the calling extension.
std::string ExecuteCommand(const DebugInterfaces* interfaces,
                           const std::string& command,
                           bool wait_for_break_status = false);

// Converts a file path to a breakpoint file path which is a path that
// uses double backslashes for the path separators.
std::string ConvertToBreakpointFilePath(const std::string& input_path,
                                        bool check_exists = false);

// A class to manage debugger context switching.
// Saves the current process and thread context on construction
// and can restore it when RestoreIfChanged() is called if the
// context has changed.
class DebugContextGuard {
 public:
  explicit DebugContextGuard(const DebugInterfaces* interfaces);
  ~DebugContextGuard() = default;

  // Restores the original process and thread context if it has
  // changed. Returns true if the current context is the same as
  // the original context. Returns false if the current context
  // does not match the original context and can not be restored.
  // false is also returned if there was any error while attempting
  // to retrieve or restore the context.
  bool RestoreIfChanged();

 private:
  const DebugInterfaces* interfaces_;
  ULONG original_process_id_;
  ULONG original_thread_id_;
  bool is_valid_;
};

// Returns the top max_depth symbols of the current call stack.
// If symbol_only is true, only symbols are returned (kc). If
// symbol_only is false all the parameters and extra details
// are returned as well (kp).
std::vector<std::string> GetTopOfCallStack(const DebugInterfaces* interfaces,
                                           size_t max_depth = 5,
                                           bool symbol_only = true);

}  // namespace utils

#endif  // UTILS_H_

// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#include "../src/utils.h"
#include "unit_test_runner.h"

// Forward declarations of globals and functions from utils.cpp
extern std::vector<std::string> utils::ParseCommandLine(const char* cmdLine);

DECLARE_TEST_RUNNER()

//
// ParseCommandLine tests
//

TEST(ParseCommandLine_Empty) {
  std::vector<std::string> args = utils::ParseCommandLine("");
  TEST_ASSERT_EQUALS(0, args.size());

  args = utils::ParseCommandLine("   ");
  TEST_ASSERT_EQUALS(0, args.size());
}

TEST(ParseCommandLine_SingleWord) {
  std::vector<std::string> args = utils::ParseCommandLine("test");
  TEST_ASSERT_EQUALS(1, args.size());
  TEST_ASSERT_EQUALS("test", args[0]);
}

TEST(ParseCommandLine_MultipleWords) {
  std::vector<std::string> args = utils::ParseCommandLine("test arg1 arg2");
  TEST_ASSERT_EQUALS(3, args.size());
  TEST_ASSERT_EQUALS("test", args[0]);
  TEST_ASSERT_EQUALS("arg1", args[1]);
  TEST_ASSERT_EQUALS("arg2", args[2]);
}

TEST(ParseCommandLine_Quoted) {
  std::vector<std::string> args =
      utils::ParseCommandLine("test 'arg with spaces' arg2");
  TEST_ASSERT_EQUALS(3, args.size());
  TEST_ASSERT_EQUALS("test", args[0]);
  TEST_ASSERT_EQUALS("arg with spaces", args[1]);
  TEST_ASSERT_EQUALS("arg2", args[2]);
}

TEST(ParseCommandLine_QuotesDelimitArgsNoSpaceBeforeOrAfter) {
  std::vector<std::string> args =
      utils::ParseCommandLine("test'arg with spaces 'arg2");
  TEST_ASSERT_EQUALS(3, args.size());
  TEST_ASSERT_EQUALS("test", args[0]);
  TEST_ASSERT_EQUALS("arg with spaces ", args[1]);
  TEST_ASSERT_EQUALS("arg2", args[2]);
}

TEST(ParseCommandLine_QuotesWithNoContentIsAnEmptyArg) {
  std::vector<std::string> args = utils::ParseCommandLine("test '' arg2");
  TEST_ASSERT_EQUALS(3, args.size());
  TEST_ASSERT_EQUALS("test", args[0]);
  TEST_ASSERT_EQUALS("", args[1]);
  TEST_ASSERT_EQUALS("arg2", args[2]);

  args = utils::ParseCommandLine("test''arg2");
  TEST_ASSERT_EQUALS(3, args.size());
  TEST_ASSERT_EQUALS("test", args[0]);
  TEST_ASSERT_EQUALS("", args[1]);
  TEST_ASSERT_EQUALS("arg2", args[2]);
}

TEST(ParseCommandLine_EscapedQuote) {
  // Backslash before a single quote escapes the quote
  std::vector<std::string> args = utils::ParseCommandLine(R"(\')");
  TEST_ASSERT_EQUALS(1, args.size());
  TEST_ASSERT_EQUALS("'", args[0]);

  args = utils::ParseCommandLine(R"(\'arg1)");
  TEST_ASSERT_EQUALS(1, args.size());
  TEST_ASSERT_EQUALS("'arg1", args[0]);

  args = utils::ParseCommandLine(R"(test\'arg1\'test)");
  TEST_ASSERT_EQUALS(1, args.size());
  TEST_ASSERT_EQUALS("test'arg1'test", args[0]);

  args = utils::ParseCommandLine(R"(test'\'arg1'\'test)");
  TEST_ASSERT_EQUALS(3, args.size());
  TEST_ASSERT_EQUALS("test", args[0]);
  TEST_ASSERT_EQUALS("'arg1", args[1]);
  TEST_ASSERT_EQUALS("'test", args[2]);
}

TEST(ParseCommandLine_DoubleEscapedQuote) {
  // The first backslash escapes the second backslash and
  // the quote is seen as an arg delimiter like normal.
  std::vector<std::string> args = utils::ParseCommandLine(R"(\\'arg1)");
  TEST_ASSERT_EQUALS(2, args.size());
  TEST_ASSERT_EQUALS("\\", args[0]);
  TEST_ASSERT_EQUALS("arg1", args[1]);

  args = utils::ParseCommandLine(R"(test\\'arg2\\'test\\arg3)");
  TEST_ASSERT_EQUALS(3, args.size());
  TEST_ASSERT_EQUALS("test\\", args[0]);
  TEST_ASSERT_EQUALS("arg2\\", args[1]);
  TEST_ASSERT_EQUALS("test\\\\arg3", args[2]);
}

TEST(ParseCommandLine_TripleEscapedQuote) {
  // The first backlash escapes the second backslash.
  // The third backslash escapes the single quote.
  // This lets the user pass a single backslash followed by a single quote.
  std::vector<std::string> args = utils::ParseCommandLine(R"(\\\'arg1)");
  TEST_ASSERT_EQUALS(1, args.size());
  TEST_ASSERT_EQUALS("\\'arg1", args[0]);

  args = utils::ParseCommandLine(R"(test\\\'arg1)");
  TEST_ASSERT_EQUALS(1, args.size());
  TEST_ASSERT_EQUALS("test\\'arg1", args[0]);
}

TEST(ParseCommandLine_QuadrupleOrMoreEscapedQuote) {
  // Anything more than three backslashes is treated as a regular backslash.
  // The first backlash is seen as a regular backslash.
  // The second backslash escapes the third backslash.
  // The fourth backslash escapes the single quote.
  std::vector<std::string> args = utils::ParseCommandLine(R"(\\\\'arg1)");
  TEST_ASSERT_EQUALS(1, args.size());
  TEST_ASSERT_EQUALS("\\\\'arg1", args[0]);

  args = utils::ParseCommandLine(R"(test\\\\'arg1)");
  TEST_ASSERT_EQUALS(1, args.size());
  TEST_ASSERT_EQUALS("test\\\\'arg1", args[0]);

  args = utils::ParseCommandLine(R"(test\\\\\'arg1)");
  TEST_ASSERT_EQUALS(1, args.size());
  TEST_ASSERT_EQUALS("test\\\\\\'arg1", args[0]);
}

TEST(ParseCommandLine_BackslashNotPrecedingQuotesHaveNoSpecialMeaning) {
  std::vector<std::string> args =
      utils::ParseCommandLine(R"(\\test\\\test2\\\\n\\\\\)");
  TEST_ASSERT_EQUALS(1, args.size());
  TEST_ASSERT_EQUALS("\\\\test\\\\\\test2\\\\\\\\n\\\\\\\\\\", args[0]);
}

TEST(
    ParseCommandLine_CharacterPrecededByBackslashPassedThroughAsIsWhenNotQuote) {
  std::vector<std::string> args =
      utils::ParseCommandLine(R"(\n\r\t\''test\'test2'arg)");
  TEST_ASSERT_EQUALS(3, args.size());
  TEST_ASSERT_EQUALS("\\n\\r\\t'", args[0]);
  TEST_ASSERT_EQUALS("test'test2", args[1]);
  TEST_ASSERT_EQUALS("arg", args[2]);
}

TEST(ParseCommandLine_DoubleQuotesPassedThroughAsIs) {
  std::vector<std::string> args =
      utils::ParseCommandLine(R"(test" 'arg "with" spaces' arg2")");
  TEST_ASSERT_EQUALS(3, args.size());
  TEST_ASSERT_EQUALS("test\"", args[0]);
  TEST_ASSERT_EQUALS("arg \"with\" spaces", args[1]);
  TEST_ASSERT_EQUALS("arg2\"", args[2]);
}

TEST(ParseCommandLine_EscapedDoubleQuotesPassedThroughAsIs) {
  std::vector<std::string> args = utils::ParseCommandLine(R"(test\"test)");
  TEST_ASSERT_EQUALS(1, args.size());
  TEST_ASSERT_EQUALS("test\\\"test", args[0]);
}

TEST(ParseCommandLine_WindowsPathPassedThroughAsIs) {
  std::vector<std::string> args =
      utils::ParseCommandLine(R"(C:\path\to\file.txt)");
  TEST_ASSERT_EQUALS(1, args.size());
  TEST_ASSERT_EQUALS("C:\\path\\to\\file.txt", args[0]);
}

TEST(ParseCommandLine_WindowsPathWithSpacesRequiresQuotes) {
  std::vector<std::string> args =
      utils::ParseCommandLine(R"(C:\path  \to\file.txt)");
  TEST_ASSERT_EQUALS(2, args.size());
  TEST_ASSERT_EQUALS("C:\\path", args[0]);
  TEST_ASSERT_EQUALS("\\to\\file.txt", args[1]);

  args = utils::ParseCommandLine(R"('C:\path  \to\file.txt')");
  TEST_ASSERT_EQUALS(1, args.size());
  TEST_ASSERT_EQUALS("C:\\path  \\to\\file.txt", args[0]);
}

TEST(ParseCommandLine_DoubleBackslashPathIsPassedThroughAsIs) {
  std::vector<std::string> args =
      utils::ParseCommandLine(R"(C:\\path\\to\\file.txt)");
  TEST_ASSERT_EQUALS(1, args.size());
  TEST_ASSERT_EQUALS("C:\\\\path\\\\to\\\\file.txt", args[0]);
}

TEST(ParseCommandLine_ForwardSlashPathPassedThroughAsIs) {
  std::vector<std::string> args =
      utils::ParseCommandLine(R"(C:/path/to/file.txt)");
  TEST_ASSERT_EQUALS(1, args.size());
  TEST_ASSERT_EQUALS("C:/path/to/file.txt", args[0]);
}

//
// ConvertToBreakpointFilePath tests
//

TEST(ConvertToBreakpointFilePath_EmptyPath) {
  std::string result = utils::ConvertToBreakpointFilePath("");
  TEST_ASSERT_EQUALS("", result);
}

TEST(ConvertToBreakpointFilePath_ForwardSlashPath) {
  std::string result =
      utils::ConvertToBreakpointFilePath("C:/Windows/System32/kernel32.dll");
  // Should convert forward slashes to double backslashes
  TEST_ASSERT_EQUALS("C:\\\\Windows\\\\System32\\\\kernel32.dll", result);
}

TEST(ConvertToBreakpointFilePath_SingleBackslashPath) {
  std::string result =
      utils::ConvertToBreakpointFilePath("C:\\Windows\\System32\\kernel32.dll");
  // Should double the backslashes
  TEST_ASSERT_EQUALS("C:\\\\Windows\\\\System32\\\\kernel32.dll", result);
}

TEST(ConvertToBreakpointFilePath_DoubleBackslashPath) {
  std::string result = utils::ConvertToBreakpointFilePath(
      "C:\\\\Windows\\\\System32\\\\kernel32.dll");
  // Should normalize to double backslashes (not quadruple)
  TEST_ASSERT_EQUALS("C:\\\\Windows\\\\System32\\\\kernel32.dll", result);
}

TEST(ConvertToBreakpointFilePath_MixedSingleAndDoubleBackslashPath) {
  std::string result = utils::ConvertToBreakpointFilePath(
      "C:\\Windows\\System32\\\\kernel32.dll");
  // Should normalize to double backslashes
  TEST_ASSERT_EQUALS("C:\\\\Windows\\\\System32\\\\kernel32.dll", result);
}

TEST(ConvertToBreakpointFilePath_MixedSlashPath) {
  std::string result =
      utils::ConvertToBreakpointFilePath("C:/Windows\\System32/kernel32.dll");
  // Should normalize all slashes to double backslashes
  TEST_ASSERT_EQUALS("C:\\\\Windows\\\\System32\\\\kernel32.dll", result);
}

TEST(ConvertToBreakpointFilePath_RelativePath) {
  // Relative paths are not supported and should return an empty string
  std::string result = utils::ConvertToBreakpointFilePath("..\\test.cpp");
  TEST_ASSERT_EQUALS("", result);
}

TEST(ConvertToBreakpointFilePath_NonExistentFile) {
  // When check_exists is true, should return
  // empty string for non-existent files
  std::string result = utils::ConvertToBreakpointFilePath(
      "C:\\this\\file\\definitely\\does\\not\\exist.cpp", true);
  TEST_ASSERT_EQUALS("", result);
}

TEST(ConvertToBreakpointFilePath_NonExistentFileNoCheck) {
  // Default behavior (check_exists is false) should
  // return path even if file doesn't exist
  std::string result = utils::ConvertToBreakpointFilePath(
      "C:\\this\\file\\definitely\\does\\not\\exist.cpp");
  TEST_ASSERT_EQUALS(
      "C:\\\\this\\\\file\\\\definitely\\\\does\\\\not\\\\exist.cpp", result);
}

TEST(ConvertToBreakpointFilePath_DirectoryPath) {
  // Should return empty string for directories regardless of check_exists
  std::string result = utils::ConvertToBreakpointFilePath("C:\\Windows");
  TEST_ASSERT_EQUALS("", result);

  // Even with check_exists true, directories should return empty
  result = utils::ConvertToBreakpointFilePath("C:\\Windows", true);
  TEST_ASSERT_EQUALS("", result);
}

TEST(ConvertToBreakpointFilePath_UNCPath) {
  // UNC paths are not supported and should return an empty string
  std::string result =
      utils::ConvertToBreakpointFilePath("\\\\server\\share\\file.cpp");
  TEST_ASSERT_EQUALS("", result);
}

TEST(ConvertToBreakpointFilePath_PathWithSpaces) {
  // Paths with spaces should be handled correctly
  std::string result =
      utils::ConvertToBreakpointFilePath("C:\\Program Files\\test.cpp");
  TEST_ASSERT_EQUALS("C:\\\\Program Files\\\\test.cpp", result);
}

TEST(ConvertToBreakpointFilePath_PathWithSpecialChars) {
  // Paths with special characters should be preserved (default check_exists =
  // false)
  std::string result =
      utils::ConvertToBreakpointFilePath("C:\\test@#$%\\file.cpp");
  TEST_ASSERT_EQUALS("C:\\\\test@#$%\\\\file.cpp", result);
}

TEST(ConvertToBreakpointFilePath_ExistingFileWithCheck) {
  // Test with an existing file when check_exists is true
  std::string result = utils::ConvertToBreakpointFilePath(
      "C:\\Windows\\System32\\kernel32.dll", true);
  // Should convert and return the path since the file exists
  TEST_ASSERT_EQUALS("C:\\\\Windows\\\\System32\\\\kernel32.dll", result);
}

int main() {
  return RUN_ALL_TESTS();
}

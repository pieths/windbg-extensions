// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#include "../src/utils.h"
#include "unit_test_runner.h"

// Forward declarations of globals and functions from utils.cpp
extern std::vector<std::string> utils::ParseCommandLine(const char* cmdLine);

DECLARE_TEST_RUNNER()

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

int main() {
  return RUN_ALL_TESTS();
}

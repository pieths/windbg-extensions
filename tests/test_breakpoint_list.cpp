// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#include "../src/breakpoint_list.h"
#include "../src/utils.h"
#include "unit_test_runner.h"

DECLARE_TEST_RUNNER()

//
// SetBreakpointsFromDelimitedString tests
//

TEST(SetBreakpointsFromDelimitedString_Empty) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString("");
  TEST_ASSERT_EQUALS(0, list.GetBreakpoints().size());

  list.SetBreakpointsFromDelimitedString("   ");
  TEST_ASSERT_EQUALS(0, list.GetBreakpoints().size());
}

TEST(SetBreakpointsFromDelimitedString_SingleBreakpoint) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString("kernel32!CreateFileW");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(1, breakpoints.size());
  TEST_ASSERT_EQUALS("kernel32!CreateFileW", breakpoints[0]);
}

TEST(SetBreakpointsFromDelimitedString_MultipleBreakpoints) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString(
      "chrome!test::testFunction,chrome!test2::testFunction2");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(2, breakpoints.size());
  TEST_ASSERT_EQUALS("chrome!test::testFunction", breakpoints[0]);
  TEST_ASSERT_EQUALS("chrome!test2::testFunction2", breakpoints[1]);
}

TEST(SetBreakpointsFromDelimitedString_BreakpointsWithSpaces) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString(
      "  kernel32!CreateFileW  ,  ntdll!NtCreateFile  ");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(2, breakpoints.size());
  TEST_ASSERT_EQUALS("kernel32!CreateFileW", breakpoints[0]);
  TEST_ASSERT_EQUALS("ntdll!NtCreateFile", breakpoints[1]);
}

TEST(SetBreakpointsFromDelimitedString_SourceLineBreakpointWithBackticks) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString("`C:\\test\\file.cpp:123`");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(1, breakpoints.size());
  TEST_ASSERT_EQUALS("`C:\\\\test\\\\file.cpp:123`", breakpoints[0]);

  list.SetBreakpointsFromDelimitedString("`chrome!C:\\test\\file.cpp:123`");
  breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(1, breakpoints.size());
  TEST_ASSERT_EQUALS("`chrome!C:\\\\test\\\\file.cpp:123`", breakpoints[0]);
}

TEST(SetBreakpointsFromDelimitedString_SourceLineBreakpointWithoutBackticks) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString("C:\\test\\file.cpp:456");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(1, breakpoints.size());
  TEST_ASSERT_EQUALS("`C:\\\\test\\\\file.cpp:456`", breakpoints[0]);

  list.SetBreakpointsFromDelimitedString("chrome!C:\\test\\file.cpp:456");
  breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(1, breakpoints.size());
  TEST_ASSERT_EQUALS("`chrome!C:\\\\test\\\\file.cpp:456`", breakpoints[0]);
}

TEST(SetBreakpointsFromDelimitedString_SourceLineBreakpointForwardSlashes) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString("C:/test/file.cpp:789");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(1, breakpoints.size());
  TEST_ASSERT_EQUALS("`C:\\\\test\\\\file.cpp:789`", breakpoints[0]);

  list.SetBreakpointsFromDelimitedString("chrome!C:/test/file.cpp:789");
  breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(1, breakpoints.size());
  TEST_ASSERT_EQUALS("`chrome!C:\\\\test\\\\file.cpp:789`", breakpoints[0]);
}

TEST(SetBreakpointsFromDelimitedString_SourceLineBreakpointDoubleBackslashes) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString("C:\\\\test\\\\file.cpp:321");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(1, breakpoints.size());
  TEST_ASSERT_EQUALS("`C:\\\\test\\\\file.cpp:321`", breakpoints[0]);

  list.SetBreakpointsFromDelimitedString("chrome!C:\\\\test\\\\file.cpp:321");
  breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(1, breakpoints.size());
  TEST_ASSERT_EQUALS("`chrome!C:\\\\test\\\\file.cpp:321`", breakpoints[0]);
}

TEST(SetBreakpointsFromDelimitedString_MultipleSourceLineBreakpoint) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString(
      "C:\\test\\file.cpp:321, C:\\\\test\\\\file.cpp:437");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(2, breakpoints.size());
  TEST_ASSERT_EQUALS("`C:\\\\test\\\\file.cpp:321`", breakpoints[0]);
  TEST_ASSERT_EQUALS("`C:\\\\test\\\\file.cpp:437`", breakpoints[1]);

  list.SetBreakpointsFromDelimitedString(
      "chrome!C:\\test\\file.cpp:321, chrome!C:\\\\test\\\\file.cpp:437");
  breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(2, breakpoints.size());
  TEST_ASSERT_EQUALS("`chrome!C:\\\\test\\\\file.cpp:321`", breakpoints[0]);
  TEST_ASSERT_EQUALS("`chrome!C:\\\\test\\\\file.cpp:437`", breakpoints[1]);
}

TEST(SetBreakpointsFromDelimitedString_MixedBreakpointTypes) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString(
      "kernel32!CreateFileW,C:\\test\\file.cpp:123,ntdll!NtCreateFile");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(3, breakpoints.size());
  TEST_ASSERT_EQUALS("kernel32!CreateFileW", breakpoints[0]);
  TEST_ASSERT_EQUALS("`C:\\\\test\\\\file.cpp:123`", breakpoints[1]);
  TEST_ASSERT_EQUALS("ntdll!NtCreateFile", breakpoints[2]);

  list.SetBreakpointsFromDelimitedString(
      "kernel32!CreateFileW,chrome!C:\\test\\file.cpp:123,ntdll!NtCreateFile");
  breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(3, breakpoints.size());
  TEST_ASSERT_EQUALS("kernel32!CreateFileW", breakpoints[0]);
  TEST_ASSERT_EQUALS("`chrome!C:\\\\test\\\\file.cpp:123`", breakpoints[1]);
  TEST_ASSERT_EQUALS("ntdll!NtCreateFile", breakpoints[2]);
}

TEST(SetBreakpointsFromDelimitedString_InvalidFilePath) {
  BreakpointList list;
  // Relative paths are not supported and should clear all breakpoints
  list.SetBreakpointsFromDelimitedString(
      "kernel32!CreateFileW,..\\test.cpp:123,ntdll!NtCreateFile");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(0, breakpoints.size());
}

TEST(SetBreakpointsFromDelimitedString_UNCPath) {
  BreakpointList list;
  // UNC paths are not supported and should clear all breakpoints
  list.SetBreakpointsFromDelimitedString("\\\\server\\share\\file.cpp:123");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(0, breakpoints.size());
}

TEST(SetBreakpointsFromDelimitedString_EmptyBreakpointsBetweenDelimiters) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString(
      "kernel32!CreateFileW,,ntdll!NtCreateFile");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(2, breakpoints.size());
  TEST_ASSERT_EQUALS("kernel32!CreateFileW", breakpoints[0]);
  TEST_ASSERT_EQUALS("ntdll!NtCreateFile", breakpoints[1]);
}

TEST(SetBreakpointsFromDelimitedString_PathWithSpaces) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString("C:\\Program Files\\test.cpp:999");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(1, breakpoints.size());
  TEST_ASSERT_EQUALS("`C:\\\\Program Files\\\\test.cpp:999`", breakpoints[0]);

  list.SetBreakpointsFromDelimitedString(
      "chrome!C:\\Program Files\\test.cpp:999");
  breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(1, breakpoints.size());
  TEST_ASSERT_EQUALS("`chrome!C:\\\\Program Files\\\\test.cpp:999`",
                     breakpoints[0]);
}

TEST(SetBreakpointsFromDelimitedString_NotASourceLineBreakpoint) {
  BreakpointList list;
  // These should not be treated as source:line breakpoints
  list.SetBreakpointsFromDelimitedString("SomeFunction:123");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(0, breakpoints.size());
}

TEST(SetBreakpointsFromDelimitedString_ModuleWithSpacesIsInvalid) {
  BreakpointList list;
  // Module names with spaces are invalid
  list.SetBreakpointsFromDelimitedString("my module!function:123");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(0, breakpoints.size());
}

int main() {
  return RUN_ALL_TESTS();
}

// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#include "../src/breakpoint.h"
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
  TEST_ASSERT_EQUALS("kernel32!CreateFileW", breakpoints[0].GetFullString());
  TEST_ASSERT_EQUALS("kernel32", breakpoints[0].GetModuleName());
  TEST_ASSERT_EQUALS("CreateFileW", breakpoints[0].GetLocation());
}

TEST(SetBreakpointsFromDelimitedString_MultipleBreakpoints) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString(
      "chrome!test::testFunction,chrome!test::testFunction2");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(2, breakpoints.size());

  TEST_ASSERT_EQUALS("chrome!test::testFunction",
                     breakpoints[0].GetFullString());
  TEST_ASSERT_EQUALS("chrome", breakpoints[0].GetModuleName());
  TEST_ASSERT_EQUALS("test::testFunction", breakpoints[0].GetLocation());

  TEST_ASSERT_EQUALS("chrome!test::testFunction2",
                     breakpoints[1].GetFullString());
  TEST_ASSERT_EQUALS("chrome", breakpoints[1].GetModuleName());
  TEST_ASSERT_EQUALS("test::testFunction2", breakpoints[1].GetLocation());
}

TEST(SetBreakpointsFromDelimitedString_BreakpointsWithSpaces) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString(
      "  kernel32!CreateFileW  ,  ntdll!NtCreateFile  ");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(2, breakpoints.size());
  TEST_ASSERT_EQUALS("kernel32!CreateFileW", breakpoints[0].GetFullString());
  TEST_ASSERT_EQUALS("ntdll!NtCreateFile", breakpoints[1].GetFullString());
}

TEST(SetBreakpointsFromDelimitedString_SourceLineBreakpointWithBackticks) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString("`chrome!C:\\test\\file.cpp:123`",
                                         "chrome");
  auto breakpoints = list.GetBreakpoints();

  TEST_ASSERT_EQUALS(1, breakpoints.size());
  TEST_ASSERT_EQUALS("`chrome!C:\\\\test\\\\file.cpp:123`",
                     breakpoints[0].GetFullString());
  TEST_ASSERT(breakpoints[0].IsSourceLineBreakpoint());
  TEST_ASSERT_EQUALS(123, breakpoints[0].GetLineNumber());
  TEST_ASSERT_EQUALS("C:\\\\test\\\\file.cpp", breakpoints[0].GetPath());
}

TEST(SetBreakpointsFromDelimitedString_SourceLineBreakpointWithoutBackticks) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString("C:\\test\\file.cpp:456", "chrome");
  auto breakpoints = list.GetBreakpoints();

  TEST_ASSERT_EQUALS(1, breakpoints.size());
  TEST_ASSERT_EQUALS("`chrome!C:\\\\test\\\\file.cpp:456`",
                     breakpoints[0].GetFullString());
  TEST_ASSERT(breakpoints[0].IsSourceLineBreakpoint());
  TEST_ASSERT_EQUALS(456, breakpoints[0].GetLineNumber());
  TEST_ASSERT_EQUALS("C:\\\\test\\\\file.cpp", breakpoints[0].GetPath());
}

TEST(SetBreakpointsFromDelimitedString_SourceLineBreakpointForwardSlashes) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString("C:/test/file.cpp:789", "chrome");
  auto breakpoints = list.GetBreakpoints();

  TEST_ASSERT_EQUALS(1, breakpoints.size());
  TEST_ASSERT_EQUALS("`chrome!C:\\\\test\\\\file.cpp:789`",
                     breakpoints[0].GetFullString());
  TEST_ASSERT(breakpoints[0].IsSourceLineBreakpoint());
  TEST_ASSERT_EQUALS(789, breakpoints[0].GetLineNumber());
  TEST_ASSERT_EQUALS("C:\\\\test\\\\file.cpp", breakpoints[0].GetPath());
}

TEST(SetBreakpointsFromDelimitedString_SourceLineBreakpointDoubleBackslashes) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString("C:\\\\test\\\\file.cpp:321",
                                         "chrome");
  auto breakpoints = list.GetBreakpoints();

  TEST_ASSERT_EQUALS(1, breakpoints.size());
  TEST_ASSERT_EQUALS("`chrome!C:\\\\test\\\\file.cpp:321`",
                     breakpoints[0].GetFullString());
  TEST_ASSERT(breakpoints[0].IsSourceLineBreakpoint());
  TEST_ASSERT_EQUALS(321, breakpoints[0].GetLineNumber());
  TEST_ASSERT_EQUALS("C:\\\\test\\\\file.cpp", breakpoints[0].GetPath());
}

TEST(SetBreakpointsFromDelimitedString_MultipleSourceLineBreakpoint) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString(
      "C:\\test\\file.cpp:321, C:\\\\test\\\\file2.cpp:437", "chrome");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(2, breakpoints.size());

  TEST_ASSERT_EQUALS("`chrome!C:\\\\test\\\\file.cpp:321`",
                     breakpoints[0].GetFullString());
  TEST_ASSERT(breakpoints[0].IsSourceLineBreakpoint());
  TEST_ASSERT_EQUALS(321, breakpoints[0].GetLineNumber());
  TEST_ASSERT_EQUALS("C:\\\\test\\\\file.cpp", breakpoints[0].GetPath());

  TEST_ASSERT_EQUALS("`chrome!C:\\\\test\\\\file2.cpp:437`",
                     breakpoints[1].GetFullString());
  TEST_ASSERT(breakpoints[1].IsSourceLineBreakpoint());
  TEST_ASSERT_EQUALS(437, breakpoints[1].GetLineNumber());
  TEST_ASSERT_EQUALS("C:\\\\test\\\\file2.cpp", breakpoints[1].GetPath());
}

TEST(SetBreakpointsFromDelimitedString_MixedBreakpointTypes) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString(
      "kernel32!CreateFileW,chrome!C:\\test\\file.cpp:123,ntdll!NtCreateFile");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(3, breakpoints.size());
  TEST_ASSERT_EQUALS("kernel32!CreateFileW", breakpoints[0].GetFullString());
  TEST_ASSERT_EQUALS("`chrome!C:\\\\test\\\\file.cpp:123`",
                     breakpoints[1].GetFullString());
  TEST_ASSERT_EQUALS("ntdll!NtCreateFile", breakpoints[2].GetFullString());
}

TEST(SetBreakpointsFromDelimitedString_InvalidFilePath) {
  BreakpointList list;
  // Relative paths are not supported and should clear all breakpoints
  list.SetBreakpointsFromDelimitedString(
      "kernel32!CreateFileW,..\\test.cpp:123,ntdll!NtCreateFile", "chrome");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(0, breakpoints.size());

  list.SetBreakpointsFromDelimitedString(
      "kernel32!CreateFileW,chrome!..\\test.cpp:123,ntdll!NtCreateFile",
      "chrome");
  breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(0, breakpoints.size());
}

TEST(SetBreakpointsFromDelimitedString_UNCPath) {
  BreakpointList list;
  // UNC paths are not supported and should clear all breakpoints
  list.SetBreakpointsFromDelimitedString("\\\\server\\share\\file.cpp:123",
                                         "chrome");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(0, breakpoints.size());
}

TEST(SetBreakpointsFromDelimitedString_EmptyBreakpointsBetweenDelimiters) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString(
      "kernel32!CreateFileW,,ntdll!NtCreateFile");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(2, breakpoints.size());
  TEST_ASSERT_EQUALS("kernel32!CreateFileW", breakpoints[0].GetFullString());
  TEST_ASSERT_EQUALS("ntdll!NtCreateFile", breakpoints[1].GetFullString());
}

TEST(SetBreakpointsFromDelimitedString_PathWithSpaces) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString(
      "chrome!C:\\Program Files\\test.cpp:999");
  auto breakpoints = list.GetBreakpoints();

  TEST_ASSERT_EQUALS(1, breakpoints.size());
  TEST_ASSERT_EQUALS("`chrome!C:\\\\Program Files\\\\test.cpp:999`",
                     breakpoints[0].GetFullString());
  TEST_ASSERT(breakpoints[0].IsSourceLineBreakpoint());
  TEST_ASSERT_EQUALS(999, breakpoints[0].GetLineNumber());
  TEST_ASSERT_EQUALS("C:\\\\Program Files\\\\test.cpp",
                     breakpoints[0].GetPath());
}

TEST(SetBreakpointsFromDelimitedString_NotASourceLineBreakpoint) {
  BreakpointList list;
  // These should not be treated as source:line breakpoints
  list.SetBreakpointsFromDelimitedString("SomeFunction:123", "chrome");
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

TEST(SetBreakpointsFromDelimitedString_WithDefaultModule) {
  BreakpointList list;
  // Test that default module is applied when no module is specified
  list.SetBreakpointsFromDelimitedString("CreateFileW,NtCreateFile",
                                         "kernel32");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(2, breakpoints.size());
  TEST_ASSERT_EQUALS("kernel32!CreateFileW", breakpoints[0].GetFullString());
  TEST_ASSERT_EQUALS("kernel32!NtCreateFile", breakpoints[1].GetFullString());
}

TEST(SetBreakpointsFromDelimitedString_PathEmptyForNonSourceLineBreakpoint) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString("kernel32!CreateFileW");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(1, breakpoints.size());
  TEST_ASSERT_EQUALS("kernel32!CreateFileW", breakpoints[0].GetFullString());
  TEST_ASSERT_EQUALS(0, breakpoints[0].GetLineNumber());
  TEST_ASSERT_EQUALS("", breakpoints[0].GetPath());
}

TEST(UpdateLineNumber_ValidSourceLineBreakpoint) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString("chrome!C:\\test\\file.cpp:123");
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(123, breakpoints[0].GetLineNumber());

  TEST_ASSERT(list.UpdateLineNumber(0, 456));
  breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS(456, breakpoints[0].GetLineNumber());
  TEST_ASSERT_EQUALS("`chrome!C:\\\\test\\\\file.cpp:456`",
                     breakpoints[0].GetFullString());
}

TEST(UpdateLineNumber_InvalidIndex) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString("chrome!C:\\test\\file.cpp:123");

  TEST_ASSERT(!list.UpdateLineNumber(1, 456));  // Index out of bounds
}

TEST(UpdateLineNumber_NotSourceLineBreakpoint) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString("kernel32!CreateFileW");

  TEST_ASSERT(!list.UpdateLineNumber(0, 456));  // Not a source line breakpoint
  TEST_ASSERT_EQUALS("kernel32!CreateFileW",
                     list.GetBreakpointAtIndex(0).GetFullString());
}

//
// JSON Serialization/Deserialization tests
//

TEST(ToJson_EmptyList) {
  BreakpointList list;
  JSON json = list.ToJson();

  TEST_ASSERT(json.contains("tag"));
  TEST_ASSERT(json.contains("breakpoints"));
  TEST_ASSERT(json["tag"].is_string());
  TEST_ASSERT(json["breakpoints"].is_array());
  TEST_ASSERT_EQUALS(0, json["breakpoints"].size());
}

TEST(ToJson_WithTagAndBreakpoints) {
  BreakpointList list;
  list.SetTag("test_tag");
  list.SetBreakpointsFromDelimitedString(
      "kernel32!CreateFileW,ntdll!NtCreateFile");

  JSON json = list.ToJson();

  TEST_ASSERT_EQUALS("test_tag", json["tag"].get<std::string>());
  TEST_ASSERT_EQUALS(2, json["breakpoints"].size());

  // Check first breakpoint
  TEST_ASSERT(json["breakpoints"][0].contains("location"));
  TEST_ASSERT(json["breakpoints"][0].contains("module_name"));
  TEST_ASSERT_EQUALS("CreateFileW",
                     json["breakpoints"][0]["location"].get<std::string>());
  TEST_ASSERT_EQUALS("kernel32",
                     json["breakpoints"][0]["module_name"].get<std::string>());

  // Check second breakpoint
  TEST_ASSERT_EQUALS("NtCreateFile",
                     json["breakpoints"][1]["location"].get<std::string>());
  TEST_ASSERT_EQUALS("ntdll",
                     json["breakpoints"][1]["module_name"].get<std::string>());
}

TEST(ToJson_SourceLineBreakpoints) {
  BreakpointList list;
  list.SetBreakpointsFromDelimitedString("chrome!C:\\test\\file.cpp:123");

  JSON json = list.ToJson();

  TEST_ASSERT_EQUALS(1, json["breakpoints"].size());
  // Source line breakpoints should store the location with line number
  TEST_ASSERT_EQUALS("C:\\\\test\\\\file.cpp:123",
                     json["breakpoints"][0]["location"].get<std::string>());
  TEST_ASSERT_EQUALS("chrome",
                     json["breakpoints"][0]["module_name"].get<std::string>());
}

TEST(FromJson_EmptyList) {
  JSON json;
  json["tag"] = "";
  json["breakpoints"] = JSON::array();

  BreakpointList list = BreakpointList::FromJson(json);

  TEST_ASSERT_EQUALS("", list.GetTag());
  TEST_ASSERT_EQUALS(0, list.GetBreakpointsCount());
}

TEST(FromJson_WithTagAndBreakpoints) {
  JSON json;
  json["tag"] = "restored_tag";
  json["breakpoints"] = JSON::array();

  JSON bp1;
  bp1["location"] = "CreateFileW";
  bp1["module_name"] = "kernel32";
  json["breakpoints"].push_back(bp1);

  JSON bp2;
  bp2["location"] = "NtCreateFile";
  bp2["module_name"] = "ntdll";
  json["breakpoints"].push_back(bp2);

  BreakpointList list = BreakpointList::FromJson(json);

  TEST_ASSERT_EQUALS("restored_tag", list.GetTag());
  TEST_ASSERT_EQUALS(2, list.GetBreakpointsCount());

  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS("kernel32!CreateFileW", breakpoints[0].GetFullString());
  TEST_ASSERT_EQUALS("ntdll!NtCreateFile", breakpoints[1].GetFullString());
}

TEST(FromJson_SourceLineBreakpoints) {
  JSON json;
  json["tag"] = "source_tag";
  json["breakpoints"] = JSON::array();

  JSON bp;
  bp["location"] = "C:\\\\test\\\\file.cpp:123";
  bp["module_name"] = "chrome";
  json["breakpoints"].push_back(bp);

  BreakpointList list = BreakpointList::FromJson(json);

  TEST_ASSERT_EQUALS(1, list.GetBreakpointsCount());
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS("`chrome!C:\\\\test\\\\file.cpp:123`",
                     breakpoints[0].GetFullString());
  TEST_ASSERT(breakpoints[0].IsSourceLineBreakpoint());
  TEST_ASSERT_EQUALS(123, breakpoints[0].GetLineNumber());
  TEST_ASSERT_EQUALS("C:\\\\test\\\\file.cpp", breakpoints[0].GetPath());
}

TEST(FromJson_MissingFields) {
  // Test with missing tag field
  JSON json1;
  json1["breakpoints"] = JSON::array();

  BreakpointList list1 = BreakpointList::FromJson(json1);
  TEST_ASSERT_EQUALS("", list1.GetTag());  // Should default to empty string

  // Test with missing breakpoints field
  JSON json2;
  json2["tag"] = "test";

  BreakpointList list2 = BreakpointList::FromJson(json2);
  TEST_ASSERT_EQUALS(
      0, list2.GetBreakpointsCount());  // Should have no breakpoints
}

TEST(FromJson_InvalidBreakpoints) {
  JSON json;
  json["tag"] = "test";
  json["breakpoints"] = JSON::array();

  // Add an invalid breakpoint (missing module_name)
  JSON invalid_bp;
  invalid_bp["location"] = "CreateFileW";
  json["breakpoints"].push_back(invalid_bp);

  // Add a valid breakpoint
  JSON valid_bp;
  valid_bp["location"] = "NtCreateFile";
  valid_bp["module_name"] = "ntdll";
  json["breakpoints"].push_back(valid_bp);

  BreakpointList list = BreakpointList::FromJson(json);

  // Should only have the valid breakpoint
  TEST_ASSERT_EQUALS(1, list.GetBreakpointsCount());
  auto breakpoints = list.GetBreakpoints();
  TEST_ASSERT_EQUALS("ntdll!NtCreateFile", breakpoints[0].GetFullString());
}

TEST(RoundTrip_ToJsonFromJson) {
  // Create original list
  BreakpointList original;
  original.SetTag("round_trip_test");
  original.SetBreakpointsFromDelimitedString(
      "kernel32!CreateFileW,chrome!C:\\test\\file.cpp:123,ntdll!NtCreateFile");

  // Convert to JSON and back
  JSON json = original.ToJson();
  BreakpointList restored = BreakpointList::FromJson(json);

  // Verify they are equal
  TEST_ASSERT_EQUALS(original.GetTag(), restored.GetTag());
  TEST_ASSERT_EQUALS(original.GetBreakpointsCount(),
                     restored.GetBreakpointsCount());
  TEST_ASSERT(original.IsEqualTo(restored));

  // Verify individual breakpoints
  auto original_bps = original.GetBreakpoints();
  auto restored_bps = restored.GetBreakpoints();

  for (size_t i = 0; i < original_bps.size(); ++i) {
    TEST_ASSERT_EQUALS(original_bps[i].GetFullString(),
                       restored_bps[i].GetFullString());
    TEST_ASSERT_EQUALS(original_bps[i].GetModuleName(),
                       restored_bps[i].GetModuleName());
    TEST_ASSERT_EQUALS(original_bps[i].GetLocation(),
                       restored_bps[i].GetLocation());
  }
}

TEST(FromJson_NonStringTag) {
  JSON json;
  json["tag"] = 123;  // Not a string
  json["breakpoints"] = JSON::array();

  BreakpointList list = BreakpointList::FromJson(json);
  TEST_ASSERT_EQUALS("", list.GetTag());  // Should default to empty string
}

TEST(FromJson_NonArrayBreakpoints) {
  JSON json;
  json["tag"] = "test";
  json["breakpoints"] = "not_an_array";  // Not an array

  BreakpointList list = BreakpointList::FromJson(json);
  TEST_ASSERT_EQUALS(0,
                     list.GetBreakpointsCount());  // Should have no breakpoints
}

int main() {
  return RUN_ALL_TESTS();
}

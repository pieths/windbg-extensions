// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#include <string>
#include <vector>

#include "../src/breakpoint_list.h"
#include "../src/utils.h"
#include "debug_interfaces_test_base.h"
#include "unit_test_runner.h"

// Forward declarations of globals and functions from breakpoints_history.cpp
extern utils::DebugInterfaces g_debug;
extern std::vector<BreakpointList> g_breakpoint_lists;
extern std::string g_breakpoint_lists_file;
extern BreakpointList g_breakpoint_list;
extern class EventCallbacks* g_event_callbacks;

extern void InitializeBreakpoints();
extern void WriteBreakpointsToFile();
extern void SetBreakpointsInternal(const std::string& breakpoints_delimited,
                                   const std::string& new_module_name,
                                   const std::string& new_tag,
                                   bool all_processes,
                                   bool run_commands);
extern HRESULT CALLBACK SetBreakpointsInternal(IDebugClient* client,
                                               const char* args);
extern HRESULT CALLBACK SetAllProcessesBreakpointsInternal(IDebugClient* client,
                                                           const char* args);
extern HRESULT CALLBACK DebugExtensionInitializeInternal(PULONG version,
                                                         PULONG flags);
extern HRESULT CALLBACK DebugExtensionUninitializeInternal();

class BreakpointsHistoryTest : public DebugInterfacesTestBase {
 public:
  explicit BreakpointsHistoryTest() : DebugInterfacesTestBase(g_debug) {
    // Clear global state
    g_breakpoint_lists.clear();
    g_breakpoint_list = BreakpointList();
    g_breakpoint_lists_file = "test_breakpoints.json";

    // Initialize some test breakpoint lists in history
    SetupTestBreakpointHistory();
  }

  ~BreakpointsHistoryTest() {
    // Clean up after test
    g_breakpoint_lists.clear();
    g_breakpoint_list = BreakpointList();
    g_event_callbacks = nullptr;
    DebugExtensionUninitializeInternal();
  }

  void SetupTestBreakpointHistory() {
    // Add some sample breakpoint lists to history
    BreakpointList list1("kernel32!ReadFile", "kernel32", "file_io");
    g_breakpoint_lists.push_back(list1);

    BreakpointList list2("kernel32!WriteFile, kernel32!CreateFileW", "kernel32",
                         "file_ops");
    g_breakpoint_lists.push_back(list2);

    BreakpointList list3("ntdll!NtCreateFile", "ntdll", "nt_file");
    g_breakpoint_lists.push_back(list3);

    BreakpointList list4("chrome!main, chrome!WinMain", "chrome",
                         "entry_points");
    g_breakpoint_lists.push_back(list4);
  }

  bool VerifyBreakpointExists(size_t list_index,
                              const std::string& expected_full_string) {
    if (list_index >= g_breakpoint_lists.size()) {
      return false;
    }
    const auto& breakpoints = g_breakpoint_lists[list_index].GetBreakpoints();
    if (breakpoints.empty()) {
      return false;
    }
    for (const auto& bp : breakpoints) {
      if (bp.GetFullString() == expected_full_string) {
        return true;
      }
    }
    return false;
  }

  bool VerifyBreakpointListSize(size_t list_index, size_t expected_size) {
    if (list_index >= g_breakpoint_lists.size()) {
      return false;
    }
    return g_breakpoint_lists[list_index].GetBreakpoints().size() ==
           expected_size;
  }

  size_t GetHistorySize() const { return g_breakpoint_lists.size(); }

  std::string GetFirstBreakpointTag() const {
    if (!g_breakpoint_lists.empty()) {
      return g_breakpoint_lists[0].GetTag();
    }
    return "";
  }
};

DECLARE_TEST_RUNNER()

// Test help functionality
TEST(SetBreakpointsInternal_Help) {
  BreakpointsHistoryTest test;

  HRESULT hr = SetBreakpointsInternal(test.mock_client, "?");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining("SetBreakpoints Usage:"));
  TEST_ASSERT(test.HasOutputContaining(
      "This function sets breakpoints in the current process only"));
}

// Test setting breakpoints from history by index
TEST(SetBreakpointsInternal_FromHistoryByIndex) {
  BreakpointsHistoryTest test;
  size_t initial_history_size = test.GetHistorySize();

  test.mock_control->SetMethodOverride(
      "Execute",
      [](ULONG OutputControl, PCSTR Command, ULONG Flags) { return S_OK; });

  HRESULT hr = SetBreakpointsInternal(test.mock_client, "1");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining(
      "Setting the following breakpoints for the current process:"));

  TEST_ASSERT(test.VerifyBreakpointListSize(0, 2));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!WriteFile"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!CreateFileW"));

  // No new breakpoint list should have been created
  TEST_ASSERT_EQUALS(initial_history_size, g_breakpoint_lists.size());
}

// Test setting breakpoints with module name replacement
TEST(SetBreakpointsInternal_NewBreakpointListWithModuleReplacement) {
  BreakpointsHistoryTest test;
  size_t initial_history_size = test.GetHistorySize();

  test.mock_control->SetMethodOverride(
      "Execute",
      [](ULONG OutputControl, PCSTR Command, ULONG Flags) { return S_OK; });

  HRESULT hr = SetBreakpointsInternal(
      test.mock_client, "chrome!test,content!test2,func3 +mymodule");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining(
      "Setting the following breakpoints for the current process:"));

  // A new breakpoint list should have been created
  TEST_ASSERT_EQUALS(initial_history_size + 1, g_breakpoint_lists.size());

  // The module names should be replaced
  TEST_ASSERT(test.VerifyBreakpointListSize(0, 3));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "mymodule!test"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "mymodule!test2"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "mymodule!func3"));
}

// Test setting breakpoints with module name replacement
TEST(SetBreakpointsInternal_ExistingBreakpointListWithModuleReplacement) {
  BreakpointsHistoryTest test;
  size_t initial_history_size = test.GetHistorySize();

  test.mock_control->SetMethodOverride(
      "Execute",
      [](ULONG OutputControl, PCSTR Command, ULONG Flags) { return S_OK; });

  HRESULT hr = SetBreakpointsInternal(test.mock_client, "1 +mymodule");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining(
      "Setting the following breakpoints for the current process:"));

  // A new breakpoint list should have been created
  TEST_ASSERT_EQUALS(initial_history_size + 1, g_breakpoint_lists.size());

  // The module names should be replaced
  TEST_ASSERT(test.VerifyBreakpointListSize(0, 2));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "mymodule!WriteFile"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "mymodule!CreateFileW"));
}

// Test setting breakpoints with module name replacement
TEST(SetBreakpointsInternal_CombinedBreakpointListWithModuleReplacement) {
  BreakpointsHistoryTest test;
  size_t initial_history_size = test.GetHistorySize();

  test.mock_control->SetMethodOverride(
      "Execute",
      [](ULONG OutputControl, PCSTR Command, ULONG Flags) { return S_OK; });

  HRESULT hr =
      SetBreakpointsInternal(test.mock_client, "1+chrome!test,func2 +mymodule");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining(
      "Setting the following breakpoints for the current process:"));

  // A new breakpoint list should have been created
  TEST_ASSERT_EQUALS(initial_history_size + 1, g_breakpoint_lists.size());

  // The module names should be replaced
  TEST_ASSERT(test.VerifyBreakpointListSize(0, 4));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "mymodule!WriteFile"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "mymodule!CreateFileW"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "mymodule!test"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "mymodule!func2"));
}

// Test setting breakpoints with module name replacement
TEST(SetBreakpointsInternal_ModuleReplacementWithExistingModuleName) {
  BreakpointsHistoryTest test;
  size_t initial_history_size = test.GetHistorySize();

  test.mock_control->SetMethodOverride(
      "Execute",
      [](ULONG OutputControl, PCSTR Command, ULONG Flags) { return S_OK; });

  HRESULT hr = SetBreakpointsInternal(test.mock_client, "1 +kernel32");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining(
      "Setting the following breakpoints for the current process:"));

  // No new breakpoint list should have been created
  TEST_ASSERT_EQUALS(initial_history_size, g_breakpoint_lists.size());

  // The module names should be the same
  TEST_ASSERT(test.VerifyBreakpointListSize(0, 2));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!WriteFile"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!CreateFileW"));
}

// Test setting breakpoints with default module name
TEST(SetBreakpointsInternal_NewBreakpointListWithDefaultModule) {
  BreakpointsHistoryTest test;
  size_t initial_history_size = test.GetHistorySize();

  test.mock_control->SetMethodOverride(
      "Execute",
      [](ULONG OutputControl, PCSTR Command, ULONG Flags) { return S_OK; });

  HRESULT hr = SetBreakpointsInternal(
      test.mock_client, "WriteFile,chrome!test::test,ReadFile tests.exe");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining(
      "Setting the following breakpoints for the current process:"));

  // A new breakpoint list should have been created
  TEST_ASSERT_EQUALS(initial_history_size + 1, g_breakpoint_lists.size());

  // The breakpoints that don't already have
  // a module name should use the default
  TEST_ASSERT(test.VerifyBreakpointListSize(0, 3));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "chrome!test::test"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "tests.exe!ReadFile"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "tests.exe!WriteFile"));
}

// Test setting breakpoints with default module name
TEST(SetBreakpointsInternal_DefaultModuleDoesNotUpdateExistingBreakpoints) {
  BreakpointsHistoryTest test;
  size_t initial_history_size = test.GetHistorySize();

  test.mock_control->SetMethodOverride(
      "Execute",
      [](ULONG OutputControl, PCSTR Command, ULONG Flags) { return S_OK; });

  HRESULT hr = SetBreakpointsInternal(test.mock_client, "1 tests.exe");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining(
      "Setting the following breakpoints for the current process:"));

  // A new breakpoint list should have been created
  TEST_ASSERT_EQUALS(initial_history_size, g_breakpoint_lists.size());

  TEST_ASSERT(test.VerifyBreakpointListSize(0, 2));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!WriteFile"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!CreateFileW"));
}

// Test dry run mode
TEST(SetBreakpointsInternal_DryRun) {
  BreakpointsHistoryTest test;
  size_t initial_history_size = test.GetHistorySize();

  HRESULT hr = SetBreakpointsInternal(test.mock_client, "! 0");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining(
      "DRY RUN. No commands executed and history has not been updated"));
  // History should not change in dry run mode
  TEST_ASSERT_EQUALS(initial_history_size, test.GetHistorySize());

  TEST_ASSERT(!test.mock_control->WasCalled("Execute"));
}

TEST(SetBreakpointsInternal_NewBreakpointNotAddedIfAlreadyExists) {
  BreakpointsHistoryTest test;
  size_t initial_history_size = test.GetHistorySize();

  test.mock_control->SetMethodOverride(
      "Execute",
      [](ULONG OutputControl, PCSTR Command, ULONG Flags) { return S_OK; });

  HRESULT hr = SetBreakpointsInternal(
      test.mock_client, "kernel32!CreateFileW,kernel32!WriteFile . file_ops");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining(
      "Setting the following breakpoints for the current process:"));

  TEST_ASSERT(test.VerifyBreakpointListSize(0, 2));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!WriteFile"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!CreateFileW"));

  TEST_ASSERT_EQUALS("file_ops", test.GetFirstBreakpointTag());

  // No new breakpoint list should have been created
  TEST_ASSERT_EQUALS(initial_history_size, test.GetHistorySize());
}

TEST(
    SetBreakpointsInternal_NewBreakpointNotAddedIfAlreadyExistsWithDefaultModuleName) {
  BreakpointsHistoryTest test;
  size_t initial_history_size = test.GetHistorySize();

  test.mock_control->SetMethodOverride(
      "Execute",
      [](ULONG OutputControl, PCSTR Command, ULONG Flags) { return S_OK; });

  HRESULT hr = SetBreakpointsInternal(
      test.mock_client, "'1 + kernel32!CreateFileW,WriteFile' kernel32");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining(
      "Setting the following breakpoints for the current process:"));

  TEST_ASSERT(test.VerifyBreakpointListSize(0, 2));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!WriteFile"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!CreateFileW"));

  // The new breakpoint list should have the same tag as the first one
  TEST_ASSERT_EQUALS("file_ops", test.GetFirstBreakpointTag());

  // No new breakpoint list should have been created
  TEST_ASSERT_EQUALS(initial_history_size, test.GetHistorySize());
}

// Test combining multiple breakpoint lists
TEST(SetBreakpointsInternal_CombineMultipleLists) {
  BreakpointsHistoryTest test;
  size_t initial_history_size = test.GetHistorySize();

  test.mock_control->SetMethodOverride(
      "Execute",
      [](ULONG OutputControl, PCSTR Command, ULONG Flags) { return S_OK; });

  HRESULT hr = SetBreakpointsInternal(test.mock_client, "'0 1 2'");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining(
      "Setting the following breakpoints for the current process:"));

  TEST_ASSERT(test.VerifyBreakpointListSize(0, 4));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!ReadFile"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!WriteFile"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!CreateFileW"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "ntdll!NtCreateFile"));

  // The new breakpoint list should have the same tag as the first one
  TEST_ASSERT_EQUALS("file_io", test.GetFirstBreakpointTag());

  // A new breakpoint list should have been created
  TEST_ASSERT_EQUALS(initial_history_size + 1, test.GetHistorySize());
}

// Test combining multiple breakpoint lists
TEST(SetBreakpointsInternal_CombineMultipleListsWithModuleReplacement) {
  BreakpointsHistoryTest test;
  size_t initial_history_size = test.GetHistorySize();

  test.mock_control->SetMethodOverride(
      "Execute",
      [](ULONG OutputControl, PCSTR Command, ULONG Flags) { return S_OK; });

  HRESULT hr = SetBreakpointsInternal(test.mock_client, "'0 1 2' +mymodule");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining(
      "Setting the following breakpoints for the current process:"));

  TEST_ASSERT(test.VerifyBreakpointExists(0, "mymodule!ReadFile"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "mymodule!WriteFile"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "mymodule!CreateFileW"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "mymodule!NtCreateFile"));

  // The new breakpoint list should have the same tag as the first one
  TEST_ASSERT_EQUALS("file_io", test.GetFirstBreakpointTag());

  // A new breakpoint list should have been created
  TEST_ASSERT_EQUALS(initial_history_size + 1, test.GetHistorySize());
}

// Test combining multiple breakpoint lists
TEST(
    SetBreakpointsInternal_CombineMultipleListsWithModuleReplacementUpdateTag) {
  BreakpointsHistoryTest test;
  size_t initial_history_size = test.GetHistorySize();

  test.mock_control->SetMethodOverride(
      "Execute",
      [](ULONG OutputControl, PCSTR Command, ULONG Flags) { return S_OK; });

  HRESULT hr =
      SetBreakpointsInternal(test.mock_client, "'0 1 2' +mymodule new_tag");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining(
      "Setting the following breakpoints for the current process:"));

  TEST_ASSERT(test.VerifyBreakpointListSize(0, 4));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "mymodule!ReadFile"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "mymodule!WriteFile"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "mymodule!CreateFileW"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "mymodule!NtCreateFile"));

  // The new breakpoint list should have the same tag as the first one
  TEST_ASSERT_EQUALS("new_tag", test.GetFirstBreakpointTag());

  // A new breakpoint list should have been created
  TEST_ASSERT_EQUALS(initial_history_size + 1, test.GetHistorySize());
}

// Test combining multiple breakpoint lists
TEST(
    SetBreakpointsInternal_CombineMultipleListsWithModuleReplacementAndNewTag) {
  BreakpointsHistoryTest test;
  size_t initial_history_size = test.GetHistorySize();

  test.mock_control->SetMethodOverride(
      "Execute",
      [](ULONG OutputControl, PCSTR Command, ULONG Flags) { return S_OK; });

  HRESULT hr =
      SetBreakpointsInternal(test.mock_client, "'0 1 2' +mymodule new_tag");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining(
      "Setting the following breakpoints for the current process:"));

  TEST_ASSERT(test.VerifyBreakpointListSize(0, 4));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "mymodule!ReadFile"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "mymodule!WriteFile"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "mymodule!CreateFileW"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "mymodule!NtCreateFile"));

  // The new breakpoint list should have the new tag
  TEST_ASSERT_EQUALS("new_tag", test.GetFirstBreakpointTag());

  // A new breakpoint list should have been created
  TEST_ASSERT_EQUALS(initial_history_size + 1, test.GetHistorySize());
}

// Test combining multiple breakpoint lists
TEST(SetBreakpointsInternal_CombineMultipleListsNoModuleReplacementAndNewTag) {
  BreakpointsHistoryTest test;
  size_t initial_history_size = test.GetHistorySize();

  test.mock_control->SetMethodOverride(
      "Execute",
      [](ULONG OutputControl, PCSTR Command, ULONG Flags) { return S_OK; });

  HRESULT hr = SetBreakpointsInternal(test.mock_client, "'0 1 2' . new_tag");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining(
      "Setting the following breakpoints for the current process:"));

  TEST_ASSERT(test.VerifyBreakpointListSize(0, 4));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!ReadFile"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!WriteFile"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!CreateFileW"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "ntdll!NtCreateFile"));

  // The new breakpoint list should have the new tag
  TEST_ASSERT_EQUALS("new_tag", test.GetFirstBreakpointTag());

  // A new breakpoint list should have been created
  TEST_ASSERT_EQUALS(initial_history_size + 1, test.GetHistorySize());
}

// Test searching by text
TEST(SetBreakpointsInternal_SearchByText) {
  BreakpointsHistoryTest test;
  size_t initial_history_size = test.GetHistorySize();

  test.mock_control->SetMethodOverride(
      "Execute",
      [](ULONG OutputControl, PCSTR Command, ULONG Flags) { return S_OK; });

  HRESULT hr = SetBreakpointsInternal(test.mock_client, "s:createfile");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining(
      "Setting the following breakpoints for the current process:"));

  // Should find both kernel32!CreateFileW and ntdll!NtCreateFile
  TEST_ASSERT(test.VerifyBreakpointListSize(0, 2));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!CreateFileW"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!WriteFile"));

  TEST_ASSERT_EQUALS(initial_history_size, test.GetHistorySize());
}

// Test searching by tag
TEST(SetBreakpointsInternal_SearchByTag) {
  BreakpointsHistoryTest test;
  size_t initial_history_size = test.GetHistorySize();

  test.mock_control->SetMethodOverride(
      "Execute",
      [](ULONG OutputControl, PCSTR Command, ULONG Flags) { return S_OK; });

  HRESULT hr = SetBreakpointsInternal(test.mock_client, "t:file_ops");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining(
      "Setting the following breakpoints for the current process:"));

  TEST_ASSERT(test.VerifyBreakpointListSize(0, 2));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!WriteFile"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!CreateFileW"));

  TEST_ASSERT_EQUALS(initial_history_size, test.GetHistorySize());
}

// Test setting new tag
TEST(SetBreakpointsInternal_SetNewTag) {
  BreakpointsHistoryTest test;
  size_t initial_history_size = test.GetHistorySize();

  test.mock_control->SetMethodOverride(
      "Execute",
      [](ULONG OutputControl, PCSTR Command, ULONG Flags) { return S_OK; });

  HRESULT hr = SetBreakpointsInternal(test.mock_client, "0 . new_tag");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT_EQUALS("new_tag", test.GetFirstBreakpointTag());

  TEST_ASSERT(test.VerifyBreakpointListSize(0, 1));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!ReadFile"));

  TEST_ASSERT_EQUALS(initial_history_size + 1, test.GetHistorySize());
}

// Test removing tag with "-"
TEST(SetBreakpointsInternal_RemoveTag) {
  BreakpointsHistoryTest test;
  size_t initial_history_size = test.GetHistorySize();

  test.mock_control->SetMethodOverride(
      "Execute",
      [](ULONG OutputControl, PCSTR Command, ULONG Flags) { return S_OK; });

  HRESULT hr = SetBreakpointsInternal(test.mock_client, "0 . -");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT_EQUALS("", test.GetFirstBreakpointTag());

  TEST_ASSERT(test.VerifyBreakpointListSize(0, 1));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!ReadFile"));

  TEST_ASSERT_EQUALS(initial_history_size + 1, test.GetHistorySize());
}

// Test invalid index
TEST(SetBreakpointsInternal_InvalidIndex) {
  BreakpointsHistoryTest test;
  size_t initial_history_size = test.GetHistorySize();

  HRESULT hr = SetBreakpointsInternal(test.mock_client, "999");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasErrorContaining("Invalid index: 999"));

  TEST_ASSERT_EQUALS(initial_history_size, test.GetHistorySize());
}

// Test empty history
TEST(SetBreakpointsInternal_EmptyHistory) {
  BreakpointsHistoryTest test;
  g_breakpoint_lists.clear();

  HRESULT hr = SetBreakpointsInternal(test.mock_client, "");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasErrorContaining("No breakpoint history available"));
}

// Test creating new breakpoints
TEST(SetBreakpointsInternal_CreateNewBreakpoints) {
  BreakpointsHistoryTest test;
  test.mock_control->SetMethodOverride(
      "Execute",
      [](ULONG OutputControl, PCSTR Command, ULONG Flags) { return S_OK; });

  HRESULT hr = SetBreakpointsInternal(test.mock_client,
                                      "user32!MessageBoxW user32.dll ui_funcs");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining(
      "Setting the following breakpoints for the current process:"));

  TEST_ASSERT(test.VerifyBreakpointListSize(0, 1));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "user32!MessageBoxW"));
  TEST_ASSERT_EQUALS("ui_funcs", test.GetFirstBreakpointTag());
}

// Test combining history with new breakpoints
TEST(SetBreakpointsInternal_CombineHistoryAndNew) {
  BreakpointsHistoryTest test;
  test.mock_control->SetMethodOverride(
      "Execute",
      [](ULONG OutputControl, PCSTR Command, ULONG Flags) { return S_OK; });

  HRESULT hr =
      SetBreakpointsInternal(test.mock_client, "'0 + user32!MessageBoxW'");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining(
      "Setting the following breakpoints for the current process:"));

  TEST_ASSERT(test.VerifyBreakpointListSize(0, 2));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!ReadFile"));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "user32!MessageBoxW"));
}

// Test SetAllProcessesBreakpoints
TEST(SetAllProcessesBreakpointsInternal_Basic) {
  BreakpointsHistoryTest test;
  test.mock_control->SetMethodOverride(
      "Execute",
      [](ULONG OutputControl, PCSTR Command, ULONG Flags) { return S_OK; });
  test.mock_client->SetMethodOverride(
      "SetEventCallbacks",
      [](IDebugEventCallbacks* callbacks) { return S_OK; });

  HRESULT hr = SetAllProcessesBreakpointsInternal(test.mock_client, "0");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining(
      "Setting the following breakpoints for all processes:"));
  TEST_ASSERT(g_event_callbacks != nullptr);
  TEST_ASSERT(g_breakpoint_list.IsValid());

  TEST_ASSERT(test.VerifyBreakpointListSize(0, 1));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!ReadFile"));
}

// Test specific breakpoint selection with dot notation
TEST(SetBreakpointsInternal_DotNotation) {
  BreakpointsHistoryTest test;
  size_t initial_history_size = test.GetHistorySize();

  test.mock_control->SetMethodOverride(
      "Execute",
      [](ULONG OutputControl, PCSTR Command, ULONG Flags) { return S_OK; });

  HRESULT hr = SetBreakpointsInternal(test.mock_client, "1.0");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining(
      "Setting the following breakpoints for the current process:"));

  TEST_ASSERT(test.VerifyBreakpointListSize(0, 1));
  TEST_ASSERT(test.VerifyBreakpointExists(0, "kernel32!WriteFile"));

  TEST_ASSERT(test.GetFirstBreakpointTag() == "file_ops");
}

// Test current location breakpoint (would need mock for
// GetScope/GetLineByOffset)
TEST(SetBreakpointsInternal_CurrentLocation) {
  BreakpointsHistoryTest test;
  size_t initial_history_size = test.GetHistorySize();

  test.mock_symbols->SetMethodOverride(
      "GetScope", [](PULONG64 InstructionOffset, PDEBUG_STACK_FRAME ScopeFrame,
                     PVOID ScopeContext, ULONG ScopeContextSize) {
        *InstructionOffset = 0x12345678;
        return S_OK;
      });
  test.mock_symbols->SetMethodOverride(
      "GetLineByOffset",
      [](ULONG64 Offset, PULONG Line, PSTR FileBuffer, ULONG FileBufferSize,
         PULONG FileSize, PULONG64 Displacement) {
        *Line = 42;
        if (FileBuffer && FileBufferSize > 0) {
          strcpy_s(FileBuffer, FileBufferSize, "c:\\testfile.cpp");
        }
        return S_OK;
      });
  test.mock_symbols->SetMethodOverride(
      "GetModuleByOffset",
      [](ULONG64 Offset, ULONG StartIndex, PULONG Index, PULONG64 Base) {
        // Return a valid module
        if (Index) {
          *Index = 0;
        }
        if (Base) {
          *Base = 0x10000000;
        }
        return S_OK;
      });
  test.mock_symbols->SetMethodOverride(
      "GetModuleNames",
      [](ULONG Index, ULONG64 Base, PSTR ImageNameBuffer,
         ULONG ImageNameBufferSize, PULONG ImageNameSize, PSTR ModuleNameBuffer,
         ULONG ModuleNameBufferSize, PULONG ModuleNameSize,
         PSTR LoadedImageNameBuffer, ULONG LoadedImageNameBufferSize,
         PULONG LoadedImageNameSize) {
        // Provide a module name
        if (LoadedImageNameBuffer && LoadedImageNameBufferSize > 0) {
          strcpy_s(LoadedImageNameBuffer, LoadedImageNameBufferSize,
                   "c:\\testmodule.dll");
        }
        if (LoadedImageNameSize) {
          *LoadedImageNameSize =
              18;  // Length of "testmodule.dll" + null terminator
        }
        return S_OK;
      });
  test.mock_control->SetMethodOverride(
      "Execute",
      [](ULONG OutputControl, PCSTR Command, ULONG Flags) { return S_OK; });

  HRESULT hr = SetBreakpointsInternal(test.mock_client, ".");
  TEST_ASSERT_EQUALS(S_OK, hr);

  TEST_ASSERT(test.VerifyBreakpointListSize(0, 1));
  TEST_ASSERT(
      test.VerifyBreakpointExists(0, "`testmodule!c:\\\\testfile.cpp:42`"));

  TEST_ASSERT_EQUALS(initial_history_size + 1, g_breakpoint_lists.size());
}

// Test DebugExtensionInitialize
TEST(DebugExtensionInitialize_Success) {
  BreakpointsHistoryTest test;
  ULONG version = 0;
  ULONG flags = 0;

  HRESULT hr = DebugExtensionInitializeInternal(&version, &flags);
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT_EQUALS(DEBUG_EXTENSION_VERSION(1, 0), version);
  TEST_ASSERT_EQUALS(0, flags);
}

int main() {
  return RUN_ALL_TESTS();
}

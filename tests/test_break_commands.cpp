// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#include <string>
#include <vector>

#include "../src/utils.h"
#include "debug_interfaces_test_base.h"
#include "unit_test_runner.h"

// Forward declarations of globals and functions from break_commands.cpp
extern utils::DebugInterfaces g_debug;
extern std::vector<std::string> g_commands;
extern class BreakEventHandler* g_break_event_handler;

extern HRESULT CALLBACK AddBreakCommandInternal(IDebugClient* client,
                                                const char* args);
extern HRESULT CALLBACK ListBreakCommandsInternal(IDebugClient* client,
                                                  const char* args);
extern HRESULT CALLBACK RemoveBreakCommandsInternal(IDebugClient* client,
                                                    const char* args);
extern HRESULT CALLBACK DebugExtensionInitializeInternal(PULONG version,
                                                         PULONG flags);
extern HRESULT CALLBACK DebugExtensionUninitializeInternal();

class BreakCommandsTest : public DebugInterfacesTestBase {
 public:
  explicit BreakCommandsTest() : DebugInterfacesTestBase(g_debug) {
    g_commands.clear();
  }

  ~BreakCommandsTest() {
    // Clean up after test
    g_commands.clear();
    DebugExtensionUninitializeInternal();
  }

  void SetupTestCommands() {
    g_commands.clear();
    g_commands.push_back("k");
    g_commands.push_back("r eax");
    g_commands.push_back("!peb");
  }
};

DECLARE_TEST_RUNNER()

TEST(AddBreakCommand_Help) {
  BreakCommandsTest test;

  HRESULT hr = AddBreakCommandInternal(test.mock_client, "?");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(
      test.HasOutputContaining("AddBreakCommand - Adds a command to execute"));
  TEST_ASSERT(test.HasOutputContaining("Usage: !AddBreakCommand <command>"));
}

TEST(AddBreakCommand_EmptyArgs) {
  BreakCommandsTest test;

  HRESULT hr = AddBreakCommandInternal(test.mock_client, "");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(
      test.HasOutputContaining("AddBreakCommand - Adds a command to execute"));
}

TEST(AddBreakCommand_SingleCommand) {
  BreakCommandsTest test;
  test.mock_client->SetMethodOverride(
      "SetEventCallbacks",
      [](IDebugEventCallbacks* callbacks) { return S_OK; });

  HRESULT hr = AddBreakCommandInternal(test.mock_client, "k");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT_EQUALS(1, g_commands.size());
  TEST_ASSERT_EQUALS("k", g_commands[0]);
  TEST_ASSERT(test.HasOutputContaining("Break command added: k"));
  TEST_ASSERT(g_break_event_handler != nullptr);
}

TEST(AddBreakCommand_MultipleCommands) {
  BreakCommandsTest test;
  test.mock_client->SetMethodOverride(
      "SetEventCallbacks",
      [](IDebugEventCallbacks* callbacks) { return S_OK; });

  AddBreakCommandInternal(test.mock_client, "k");
  AddBreakCommandInternal(test.mock_client, "r eax");
  AddBreakCommandInternal(test.mock_client, "!peb");

  TEST_ASSERT_EQUALS(3, g_commands.size());
  TEST_ASSERT_EQUALS("k", g_commands[0]);
  TEST_ASSERT_EQUALS("r eax", g_commands[1]);
  TEST_ASSERT_EQUALS("!peb", g_commands[2]);
}

TEST(AddBreakCommand_SetEventCallbackFails) {
  BreakCommandsTest test;
  test.mock_client->SetMethodOverride(
      "SetEventCallbacks",
      [](IDebugEventCallbacks* callbacks) { return E_FAIL; });

  HRESULT hr = AddBreakCommandInternal(test.mock_client, "k");
  TEST_ASSERT_EQUALS(E_FAIL, hr);
  TEST_ASSERT_EQUALS(0, g_commands.size());
  TEST_ASSERT(g_break_event_handler == nullptr);
}

TEST(ListBreakCommands_Empty) {
  BreakCommandsTest test;

  HRESULT hr = ListBreakCommandsInternal(test.mock_client, nullptr);
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining("No break commands are currently set"));
}

TEST(ListBreakCommands_WithCommands) {
  BreakCommandsTest test;
  test.SetupTestCommands();

  HRESULT hr = ListBreakCommandsInternal(test.mock_client, nullptr);
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(test.HasOutputContaining("Current break commands:"));
  TEST_ASSERT(test.HasOutputContaining("0) k"));
  TEST_ASSERT(test.HasOutputContaining("1) r eax"));
  TEST_ASSERT(test.HasOutputContaining("2) !peb"));
}

TEST(RemoveBreakCommands_Help) {
  BreakCommandsTest test;

  HRESULT hr = RemoveBreakCommandsInternal(test.mock_client, "?");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(
      test.HasOutputContaining("RemoveBreakCommands - Removes break commands"));
  TEST_ASSERT(test.HasOutputContaining("Usage: !RemoveBreakCommands [index]"));
}

TEST(RemoveBreakCommands_All) {
  BreakCommandsTest test;
  test.SetupTestCommands();

  HRESULT hr = RemoveBreakCommandsInternal(test.mock_client, nullptr);
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT_EQUALS(0, g_commands.size());
  TEST_ASSERT(test.HasOutputContaining("Removed all 3 break commands"));
}

TEST(RemoveBreakCommands_ByIndex) {
  BreakCommandsTest test;
  test.SetupTestCommands();

  HRESULT hr = RemoveBreakCommandsInternal(test.mock_client, "1");
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT_EQUALS(2, g_commands.size());
  TEST_ASSERT_EQUALS("k", g_commands[0]);
  TEST_ASSERT_EQUALS("!peb", g_commands[1]);
  TEST_ASSERT(test.HasOutputContaining("Removed break command [1]: r eax"));
}

TEST(RemoveBreakCommands_InvalidIndex) {
  BreakCommandsTest test;
  test.SetupTestCommands();

  HRESULT hr = RemoveBreakCommandsInternal(test.mock_client, "999");
  TEST_ASSERT_EQUALS(E_INVALIDARG, hr);
  TEST_ASSERT_EQUALS(3, g_commands.size());  // No change
  TEST_ASSERT(test.HasErrorContaining("Index 999 is out of range"));
}

TEST(RemoveBreakCommands_InvalidArgument) {
  BreakCommandsTest test;
  test.SetupTestCommands();

  HRESULT hr = RemoveBreakCommandsInternal(test.mock_client, "not_a_number");
  TEST_ASSERT_EQUALS(E_INVALIDARG, hr);
  TEST_ASSERT_EQUALS(3, g_commands.size());  // No change
  TEST_ASSERT(test.HasErrorContaining("Invalid argument"));
}

TEST(DebugExtensionInitialize_Success) {
  BreakCommandsTest test;
  ULONG version = 0;
  ULONG flags = 0;

  HRESULT hr = DebugExtensionInitializeInternal(&version, &flags);
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT_EQUALS(DEBUG_EXTENSION_VERSION(1, 0), version);
  TEST_ASSERT_EQUALS(0, flags);
}

TEST(DebugExtensionUninitialize_WithEventHandler) {
  BreakCommandsTest test;
  test.mock_client->SetMethodOverride(
      "SetEventCallbacks",
      [](IDebugEventCallbacks* callbacks) { return S_OK; });

  // Create an event handler
  AddBreakCommandInternal(test.mock_client, "k");
  TEST_ASSERT(g_break_event_handler != nullptr);

  HRESULT hr = DebugExtensionUninitializeInternal();
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(g_break_event_handler == nullptr);
}

TEST(DebugExtensionUninitialize_NoEventHandler) {
  BreakCommandsTest test;

  HRESULT hr = DebugExtensionUninitializeInternal();
  TEST_ASSERT_EQUALS(S_OK, hr);
  TEST_ASSERT(g_break_event_handler == nullptr);
}

int main() {
  return RUN_ALL_TESTS();
}

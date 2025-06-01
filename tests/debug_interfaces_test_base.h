// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#ifndef DEBUG_INTERFACES_TEST_BASE_H
#define DEBUG_INTERFACES_TEST_BASE_H

#include <string>
#include <vector>
#include "../src/utils.h"
#include "mock_debug_interfaces.h"

class DebugInterfacesTestBase {
 public:
  MockDebugClient* mock_client;
  MockDebugControl* mock_control;
  MockDebugSymbols* mock_symbols;

  std::vector<std::string> output_normal;
  std::vector<std::string> output_error;

  explicit DebugInterfacesTestBase(utils::DebugInterfaces& interfaces) {
    mock_client = new MockDebugClient();
    mock_control = new MockDebugControl();
    mock_symbols = new MockDebugSymbols();

    SetupDebugInterfaces(interfaces);
    SetupOutputCapture();
  }

  // Don't allow calling any other constructors
  DebugInterfacesTestBase() = delete;
  DebugInterfacesTestBase(const DebugInterfacesTestBase&) = delete;
  DebugInterfacesTestBase(DebugInterfacesTestBase&&) = delete;
  DebugInterfacesTestBase& operator=(const DebugInterfacesTestBase&) = delete;
  DebugInterfacesTestBase& operator=(DebugInterfacesTestBase&&) = delete;

  virtual ~DebugInterfacesTestBase() {
    delete mock_client;
    delete mock_control;
    delete mock_symbols;
  }

  void ClearOutput() {
    output_normal.clear();
    output_error.clear();
  }

  bool HasOutputContaining(const std::string& text) {
    for (const auto& output : output_normal) {
      if (output.find(text) != std::string::npos) {
        return true;
      }
    }
    return false;
  }

  bool HasErrorContaining(const std::string& text) {
    for (const auto& output : output_error) {
      if (output.find(text) != std::string::npos) {
        return true;
      }
    }
    return false;
  }

 protected:
  virtual void SetupDebugInterfaces(utils::DebugInterfaces& interfaces) {
    interfaces.client = mock_client;
    interfaces.control = mock_control;
    interfaces.symbols = mock_symbols;
  }

  virtual void SetupOutputCapture() {
    mock_control->SetMethodOverride(
        "Output", [this](ULONG Mask, PCSTR Format, va_list args) -> HRESULT {
          char buffer[1024];
          vsnprintf(buffer, sizeof(buffer), Format, args);
          std::string output(buffer);

          if (Mask & DEBUG_OUTPUT_NORMAL) {
            output_normal.push_back(output);
          }
          if (Mask & DEBUG_OUTPUT_ERROR) {
            output_error.push_back(output);
          }

          return S_OK;
        });

    mock_control->SetMethodOverride(
        "ControlledOutput",
        [this](ULONG OutputControl, ULONG Mask, PCSTR Format,
               va_list args) -> HRESULT {
          char buffer[1024];
          vsnprintf(buffer, sizeof(buffer), Format, args);
          std::string output(buffer);

          if (Mask & DEBUG_OUTPUT_NORMAL) {
            output_normal.push_back(output);
          }
          if (Mask & DEBUG_OUTPUT_ERROR) {
            output_error.push_back(output);
          }

          return S_OK;
        });
  }
};

#endif  // DEBUG_INTERFACES_TEST_BASE_H

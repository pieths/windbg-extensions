// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#ifndef DEBUG_INTERFACES_TEST_BASE_H
#define DEBUG_INTERFACES_TEST_BASE_H

#include <string>
#include <vector>

#include "../src/utils.h"
#include "mocks/mock_debug_client.h"
#include "mocks/mock_debug_control.h"
#include "mocks/mock_debug_data_spaces.h"
#include "mocks/mock_debug_registers.h"
#include "mocks/mock_debug_symbols.h"
#include "mocks/mock_debug_system_objects.h"

class DebugInterfacesTestBase {
 public:
  MockDebugClient* mock_client;
  MockDebugControl* mock_control;
  MockDebugSymbols* mock_symbols;
  MockDebugDataSpaces* mock_data_spaces;
  MockDebugSystemObjects* mock_system_objects;
  MockDebugRegisters* mock_registers;

  std::vector<std::string> output_normal;
  std::vector<std::string> output_error;

  explicit DebugInterfacesTestBase(utils::DebugInterfaces& interfaces) {
    mock_client = new MockDebugClient();
    mock_control = new MockDebugControl();
    mock_symbols = new MockDebugSymbols();
    mock_data_spaces = new MockDebugDataSpaces();
    mock_system_objects = new MockDebugSystemObjects();
    mock_registers = new MockDebugRegisters();

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
    delete mock_data_spaces;
    delete mock_system_objects;
    delete mock_registers;
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
    interfaces.data_spaces = mock_data_spaces;
    interfaces.system_objects = mock_system_objects;
    interfaces.registers = mock_registers;
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

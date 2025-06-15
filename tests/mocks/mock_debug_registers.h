// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#ifndef MOCK_DEBUG_REGISTERS_H
#define MOCK_DEBUG_REGISTERS_H

#include "mock_debug_interface_base.h"

class MockDebugRegisters : public MockDebugInterfaceBase<IDebugRegisters> {
 public:
  STDMETHOD(GetNumberRegisters)(PULONG Number) override {
    return MockMethod<HRESULT>("GetNumberRegisters", Number);
  }

  STDMETHOD(GetDescription)(ULONG Register,
                            PSTR NameBuffer,
                            ULONG NameBufferSize,
                            PULONG NameSize,
                            PDEBUG_REGISTER_DESCRIPTION Desc) override {
    return MockMethod<HRESULT>("GetDescription", Register, NameBuffer,
                               NameBufferSize, NameSize, Desc);
  }

  STDMETHOD(GetIndexByName)(PCSTR Name, PULONG Index) override {
    return MockMethod<HRESULT>("GetIndexByName", Name, Index);
  }

  STDMETHOD(GetValue)(ULONG Register, PDEBUG_VALUE Value) override {
    return MockMethod<HRESULT>("GetValue", Register, Value);
  }

  STDMETHOD(SetValue)(ULONG Register, PDEBUG_VALUE Value) override {
    return MockMethod<HRESULT>("SetValue", Register, Value);
  }

  STDMETHOD(GetValues)(ULONG Count,
                       PULONG Indices,
                       ULONG Start,
                       PDEBUG_VALUE Values) override {
    return MockMethod<HRESULT>("GetValues", Count, Indices, Start, Values);
  }

  STDMETHOD(SetValues)(ULONG Count,
                       PULONG Indices,
                       ULONG Start,
                       PDEBUG_VALUE Values) override {
    return MockMethod<HRESULT>("SetValues", Count, Indices, Start, Values);
  }

  STDMETHOD(OutputRegisters)(ULONG OutputControl, ULONG Flags) override {
    return MockMethod<HRESULT>("OutputRegisters", OutputControl, Flags);
  }

  STDMETHOD(GetInstructionOffset)(PULONG64 Offset) override {
    return MockMethod<HRESULT>("GetInstructionOffset", Offset);
  }

  STDMETHOD(GetStackOffset)(PULONG64 Offset) override {
    return MockMethod<HRESULT>("GetStackOffset", Offset);
  }

  STDMETHOD(GetFrameOffset)(PULONG64 Offset) override {
    return MockMethod<HRESULT>("GetFrameOffset", Offset);
  }
};

#endif  // MOCK_DEBUG_REGISTERS_H

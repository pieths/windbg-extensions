// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#ifndef MOCK_DEBUG_SYSTEM_OBJECTS_H
#define MOCK_DEBUG_SYSTEM_OBJECTS_H

#include "mock_debug_interface_base.h"

class MockDebugSystemObjects
    : public MockDebugInterfaceBase<IDebugSystemObjects> {
 public:
  STDMETHOD(GetEventThread)(PULONG Id) override {
    return MockMethod<HRESULT>("GetEventThread", Id);
  }

  STDMETHOD(GetEventProcess)(PULONG Id) override {
    return MockMethod<HRESULT>("GetEventProcess", Id);
  }

  STDMETHOD(GetCurrentThreadId)(PULONG Id) override {
    return MockMethod<HRESULT>("GetCurrentThreadId", Id);
  }

  STDMETHOD(SetCurrentThreadId)(ULONG Id) override {
    return MockMethod<HRESULT>("SetCurrentThreadId", Id);
  }

  STDMETHOD(GetCurrentProcessId)(PULONG Id) override {
    return MockMethod<HRESULT>("GetCurrentProcessId", Id);
  }

  STDMETHOD(SetCurrentProcessId)(ULONG Id) override {
    return MockMethod<HRESULT>("SetCurrentProcessId", Id);
  }

  STDMETHOD(GetNumberThreads)(PULONG Number) override {
    return MockMethod<HRESULT>("GetNumberThreads", Number);
  }

  STDMETHOD(GetTotalNumberThreads)(PULONG Total,
                                   PULONG LargestProcess) override {
    return MockMethod<HRESULT>("GetTotalNumberThreads", Total, LargestProcess);
  }

  STDMETHOD(GetThreadIdsByIndex)(ULONG Start,
                                 ULONG Count,
                                 PULONG Ids,
                                 PULONG SysIds) override {
    return MockMethod<HRESULT>("GetThreadIdsByIndex", Start, Count, Ids,
                               SysIds);
  }

  STDMETHOD(GetThreadIdByProcessor)(ULONG Processor, PULONG Id) override {
    return MockMethod<HRESULT>("GetThreadIdByProcessor", Processor, Id);
  }

  STDMETHOD(GetCurrentThreadDataOffset)(PULONG64 Offset) override {
    return MockMethod<HRESULT>("GetCurrentThreadDataOffset", Offset);
  }

  STDMETHOD(GetThreadIdByDataOffset)(ULONG64 Offset, PULONG Id) override {
    return MockMethod<HRESULT>("GetThreadIdByDataOffset", Offset, Id);
  }

  STDMETHOD(GetCurrentThreadTeb)(PULONG64 Offset) override {
    return MockMethod<HRESULT>("GetCurrentThreadTeb", Offset);
  }

  STDMETHOD(GetThreadIdByTeb)(ULONG64 Offset, PULONG Id) override {
    return MockMethod<HRESULT>("GetThreadIdByTeb", Offset, Id);
  }

  STDMETHOD(GetCurrentThreadSystemId)(PULONG SysId) override {
    return MockMethod<HRESULT>("GetCurrentThreadSystemId", SysId);
  }

  STDMETHOD(GetThreadIdBySystemId)(ULONG SysId, PULONG Id) override {
    return MockMethod<HRESULT>("GetThreadIdBySystemId", SysId, Id);
  }

  STDMETHOD(GetCurrentThreadHandle)(PULONG64 Handle) override {
    return MockMethod<HRESULT>("GetCurrentThreadHandle", Handle);
  }

  STDMETHOD(GetThreadIdByHandle)(ULONG64 Handle, PULONG Id) override {
    return MockMethod<HRESULT>("GetThreadIdByHandle", Handle, Id);
  }

  STDMETHOD(GetNumberProcesses)(PULONG Number) override {
    return MockMethod<HRESULT>("GetNumberProcesses", Number);
  }

  STDMETHOD(GetProcessIdsByIndex)(ULONG Start,
                                  ULONG Count,
                                  PULONG Ids,
                                  PULONG SysIds) override {
    return MockMethod<HRESULT>("GetProcessIdsByIndex", Start, Count, Ids,
                               SysIds);
  }

  STDMETHOD(GetCurrentProcessDataOffset)(PULONG64 Offset) override {
    return MockMethod<HRESULT>("GetCurrentProcessDataOffset", Offset);
  }

  STDMETHOD(GetProcessIdByDataOffset)(ULONG64 Offset, PULONG Id) override {
    return MockMethod<HRESULT>("GetProcessIdByDataOffset", Offset, Id);
  }

  STDMETHOD(GetCurrentProcessPeb)(PULONG64 Offset) override {
    return MockMethod<HRESULT>("GetCurrentProcessPeb", Offset);
  }

  STDMETHOD(GetProcessIdByPeb)(ULONG64 Offset, PULONG Id) override {
    return MockMethod<HRESULT>("GetProcessIdByPeb", Offset, Id);
  }

  STDMETHOD(GetCurrentProcessSystemId)(PULONG SysId) override {
    return MockMethod<HRESULT>("GetCurrentProcessSystemId", SysId);
  }

  STDMETHOD(GetProcessIdBySystemId)(ULONG SysId, PULONG Id) override {
    return MockMethod<HRESULT>("GetProcessIdBySystemId", SysId, Id);
  }

  STDMETHOD(GetCurrentProcessHandle)(PULONG64 Handle) override {
    return MockMethod<HRESULT>("GetCurrentProcessHandle", Handle);
  }

  STDMETHOD(GetProcessIdByHandle)(ULONG64 Handle, PULONG Id) override {
    return MockMethod<HRESULT>("GetProcessIdByHandle", Handle, Id);
  }

  STDMETHOD(GetCurrentProcessExecutableName)(PSTR Buffer,
                                             ULONG BufferSize,
                                             PULONG ExeSize) override {
    return MockMethod<HRESULT>("GetCurrentProcessExecutableName", Buffer,
                               BufferSize, ExeSize);
  }
};

#endif  // MOCK_DEBUG_SYSTEM_OBJECTS_H

// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#ifndef MOCK_DEBUG_CLIENT_H
#define MOCK_DEBUG_CLIENT_H

#include "mock_debug_interface_base.h"

class MockDebugClient : public MockDebugInterfaceBase<IDebugClient> {
 public:
  STDMETHOD(AttachKernel)(ULONG Flags, PCSTR ConnectOptions) override {
    return MockMethod<HRESULT>("AttachKernel", Flags, ConnectOptions);
  }

  STDMETHOD(GetKernelConnectionOptions)(PSTR Buffer,
                                        ULONG BufferSize,
                                        PULONG OptionsSize) override {
    return MockMethod<HRESULT>("GetKernelConnectionOptions", Buffer, BufferSize,
                               OptionsSize);
  }

  STDMETHOD(SetKernelConnectionOptions)(PCSTR Options) override {
    return MockMethod<HRESULT>("SetKernelConnectionOptions", Options);
  }

  STDMETHOD(StartProcessServer)(ULONG Flags,
                                PCSTR Options,
                                PVOID Reserved) override {
    return MockMethod<HRESULT>("StartProcessServer", Flags, Options, Reserved);
  }

  STDMETHOD(ConnectProcessServer)(PCSTR RemoteOptions,
                                  PULONG64 Server) override {
    return MockMethod<HRESULT>("ConnectProcessServer", RemoteOptions, Server);
  }

  STDMETHOD(DisconnectProcessServer)(ULONG64 Server) override {
    return MockMethod<HRESULT>("DisconnectProcessServer", Server);
  }

  STDMETHOD(GetRunningProcessSystemIds)(ULONG64 Server,
                                        PULONG Ids,
                                        ULONG Count,
                                        PULONG ActualCount) override {
    return MockMethod<HRESULT>("GetRunningProcessSystemIds", Server, Ids, Count,
                               ActualCount);
  }

  STDMETHOD(GetRunningProcessSystemIdByExecutableName)(ULONG64 Server,
                                                       PCSTR ExeName,
                                                       ULONG Flags,
                                                       PULONG Id) override {
    return MockMethod<HRESULT>("GetRunningProcessSystemIdByExecutableName",
                               Server, ExeName, Flags, Id);
  }

  STDMETHOD(GetRunningProcessDescription)(
      ULONG64 Server,
      ULONG SystemId,
      ULONG Flags,
      PSTR ExeName,
      ULONG ExeNameSize,
      PULONG ActualExeNameSize,
      PSTR Description,
      ULONG DescriptionSize,
      PULONG ActualDescriptionSize) override {
    return MockMethod<HRESULT>("GetRunningProcessDescription", Server, SystemId,
                               Flags, ExeName, ExeNameSize, ActualExeNameSize,
                               Description, DescriptionSize,
                               ActualDescriptionSize);
  }

  STDMETHOD(AttachProcess)(ULONG64 Server,
                           ULONG ProcessId,
                           ULONG AttachFlags) override {
    return MockMethod<HRESULT>("AttachProcess", Server, ProcessId, AttachFlags);
  }

  STDMETHOD(CreateProcess)(ULONG64 Server,
                           PSTR CommandLine,
                           ULONG CreateFlags) override {
    return MockMethod<HRESULT>("CreateProcess", Server, CommandLine,
                               CreateFlags);
  }

  STDMETHOD(CreateProcessAndAttach)(ULONG64 Server,
                                    PSTR CommandLine,
                                    ULONG CreateFlags,
                                    ULONG ProcessId,
                                    ULONG AttachFlags) override {
    return MockMethod<HRESULT>("CreateProcessAndAttach", Server, CommandLine,
                               CreateFlags, ProcessId, AttachFlags);
  }

  STDMETHOD(GetProcessOptions)(PULONG Options) override {
    return MockMethod<HRESULT>("GetProcessOptions", Options);
  }

  STDMETHOD(AddProcessOptions)(ULONG Options) override {
    return MockMethod<HRESULT>("AddProcessOptions", Options);
  }

  STDMETHOD(RemoveProcessOptions)(ULONG Options) override {
    return MockMethod<HRESULT>("RemoveProcessOptions", Options);
  }

  STDMETHOD(SetProcessOptions)(ULONG Options) override {
    return MockMethod<HRESULT>("SetProcessOptions", Options);
  }

  STDMETHOD(OpenDumpFile)(PCSTR DumpFile) override {
    return MockMethod<HRESULT>("OpenDumpFile", DumpFile);
  }

  STDMETHOD(WriteDumpFile)(PCSTR DumpFile, ULONG Qualifier) override {
    return MockMethod<HRESULT>("WriteDumpFile", DumpFile, Qualifier);
  }

  STDMETHOD(ConnectSession)(ULONG Flags, ULONG HistoryLimit) override {
    return MockMethod<HRESULT>("ConnectSession", Flags, HistoryLimit);
  }

  STDMETHOD(StartServer)(PCSTR Options) override {
    return MockMethod<HRESULT>("StartServer", Options);
  }

  STDMETHOD(OutputServers)(ULONG OutputControl,
                           PCSTR Machine,
                           ULONG Flags) override {
    return MockMethod<HRESULT>("OutputServers", OutputControl, Machine, Flags);
  }

  STDMETHOD(TerminateProcesses)() override {
    return MockMethod<HRESULT>("TerminateProcesses");
  }

  STDMETHOD(DetachProcesses)() override {
    return MockMethod<HRESULT>("DetachProcesses");
  }

  STDMETHOD(EndSession)(ULONG Flags) override {
    return MockMethod<HRESULT>("EndSession", Flags);
  }

  STDMETHOD(GetExitCode)(PULONG Code) override {
    return MockMethod<HRESULT>("GetExitCode", Code);
  }

  STDMETHOD(DispatchCallbacks)(ULONG Timeout) override {
    return MockMethod<HRESULT>("DispatchCallbacks", Timeout);
  }

  STDMETHOD(ExitDispatch)(PDEBUG_CLIENT Client) override {
    return MockMethod<HRESULT>("ExitDispatch", Client);
  }

  STDMETHOD(CreateClient)(PDEBUG_CLIENT* Client) override {
    return MockMethod<HRESULT>("CreateClient", Client);
  }

  STDMETHOD(GetInputCallbacks)(PDEBUG_INPUT_CALLBACKS* Callbacks) override {
    return MockMethod<HRESULT>("GetInputCallbacks", Callbacks);
  }

  STDMETHOD(SetInputCallbacks)(PDEBUG_INPUT_CALLBACKS Callbacks) override {
    return MockMethod<HRESULT>("SetInputCallbacks", Callbacks);
  }

  STDMETHOD(GetOutputCallbacks)(PDEBUG_OUTPUT_CALLBACKS* Callbacks) override {
    return MockMethod<HRESULT>("GetOutputCallbacks", Callbacks);
  }

  STDMETHOD(SetOutputCallbacks)(PDEBUG_OUTPUT_CALLBACKS Callbacks) override {
    return MockMethod<HRESULT>("SetOutputCallbacks", Callbacks);
  }

  STDMETHOD(GetOutputMask)(PULONG Mask) override {
    return MockMethod<HRESULT>("GetOutputMask", Mask);
  }

  STDMETHOD(SetOutputMask)(ULONG Mask) override {
    return MockMethod<HRESULT>("SetOutputMask", Mask);
  }

  STDMETHOD(GetOtherOutputMask)(PDEBUG_CLIENT Client, PULONG Mask) override {
    return MockMethod<HRESULT>("GetOtherOutputMask", Client, Mask);
  }

  STDMETHOD(SetOtherOutputMask)(PDEBUG_CLIENT Client, ULONG Mask) override {
    return MockMethod<HRESULT>("SetOtherOutputMask", Client, Mask);
  }

  STDMETHOD(GetOutputWidth)(PULONG Columns) override {
    return MockMethod<HRESULT>("GetOutputWidth", Columns);
  }

  STDMETHOD(SetOutputWidth)(ULONG Columns) override {
    return MockMethod<HRESULT>("SetOutputWidth", Columns);
  }

  STDMETHOD(GetOutputLinePrefix)(PSTR Buffer,
                                 ULONG BufferSize,
                                 PULONG PrefixSize) override {
    return MockMethod<HRESULT>("GetOutputLinePrefix", Buffer, BufferSize,
                               PrefixSize);
  }

  STDMETHOD(SetOutputLinePrefix)(PCSTR Prefix) override {
    return MockMethod<HRESULT>("SetOutputLinePrefix", Prefix);
  }

  STDMETHOD(GetIdentity)(PSTR Buffer,
                         ULONG BufferSize,
                         PULONG IdentitySize) override {
    return MockMethod<HRESULT>("GetIdentity", Buffer, BufferSize, IdentitySize);
  }

  STDMETHOD(OutputIdentity)(ULONG OutputControl,
                            ULONG Flags,
                            PCSTR Format) override {
    return MockMethod<HRESULT>("OutputIdentity", OutputControl, Flags, Format);
  }

  STDMETHOD(GetEventCallbacks)(PDEBUG_EVENT_CALLBACKS* Callbacks) override {
    return MockMethod<HRESULT>("GetEventCallbacks", Callbacks);
  }

  STDMETHOD(SetEventCallbacks)(PDEBUG_EVENT_CALLBACKS Callbacks) override {
    return MockMethod<HRESULT>("SetEventCallbacks", Callbacks);
  }

  STDMETHOD(FlushCallbacks)() override {
    return MockMethod<HRESULT>("FlushCallbacks");
  }
};

#endif  // MOCK_DEBUG_CLIENT_H

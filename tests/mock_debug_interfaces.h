// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#ifndef MOCK_DEBUG_INTERFACES_H
#define MOCK_DEBUG_INTERFACES_H

#include <dbgeng.h>
#include <windows.h>
#include <any>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

template <typename TInterface>
class MockBase : public TInterface {
 public:
  // Type-safe version that requires explicit function signature.
  // The std::function must have the exact signature expected by the mock
  // method.
  //
  //    mock->SetMethodOverride<HRESULT(ULONG, PCSTR, va_list)>("Output",
  //      [](ULONG mask, PCSTR format, va_list args) { return S_OK; });
  //
  template <typename TSignature>
  void SetMethodOverride(const std::string& methodName,
                         std::function<TSignature> func) {
    methodOverrides[methodName] = std::move(func);
  }

  // Convenience overload that accepts any callable (lambda, function pointer,
  // etc.) and wraps it in a std::function before storing. The callable's
  // signature must match exactly what the mock method expects when it performs
  // the any_cast.
  //
  //    mock->SetMethodOverride("Output", [](ULONG mask, PCSTR format, va_list
  //        args) {
  //      return S_OK;
  //    });
  //
  template <typename TCallable>
  void SetMethodOverride(const std::string& methodName, TCallable&& callable) {
    // Explicitly wrap the callable in std::function to ensure consistent
    // storage type
    methodOverrides[methodName] =
        std::function(std::forward<TCallable>(callable));
  }

  void ClearOverrides() { methodOverrides.clear(); }

  // IUnknown methods
  STDMETHOD(QueryInterface)(REFIID InterfaceId, PVOID* Interface) override {
    return E_NOINTERFACE;
  }
  STDMETHOD_(ULONG, AddRef)() override { return 1; }
  STDMETHOD_(ULONG, Release)() override { return 1; }

  const std::vector<std::string>& GetCallHistory() const {
    return m_call_history;
  }

  // Helper method to check if a method was called
  bool WasCalled(const std::string& methodName) const {
    return std::find(m_call_history.begin(), m_call_history.end(),
                     methodName) != m_call_history.end();
  }

  // Helper method to count how many times a method was called
  size_t GetCallCount(const std::string& methodName) const {
    return std::count(m_call_history.begin(), m_call_history.end(), methodName);
  }

  void ClearCallHistory() { m_call_history.clear(); }

 protected:
  std::unordered_map<std::string, std::any> methodOverrides;

  // Template helper for regular methods
  template <typename TReturn, typename... TArgs>
  TReturn MockMethod(const char* methodName, TArgs... args) {
    m_call_history.push_back(methodName);

    auto it = methodOverrides.find(methodName);
    if (it != methodOverrides.end()) {
      using FuncType = std::function<TReturn(TArgs...)>;
      if (auto* func = std::any_cast<FuncType>(&it->second)) {
        return (*func)(args...);
      }
    }

    throw std::runtime_error(
        std::string("Mock method '") + methodName +
        "' called but not implemented. Call SetMethodOverride to define behavior.");
  }

  // Use this for variadic methods since templates
  // don't appear to work well with va_start
  template <typename TFunc>
  TFunc* GetOverride(const std::string& methodName) {
    m_call_history.push_back(methodName);

    auto it = methodOverrides.find(methodName);
    if (it != methodOverrides.end()) {
      if (auto* func = std::any_cast<TFunc>(&it->second)) {
        return func;
      }
    }

    throw std::runtime_error(
        std::string("Mock method '") + methodName +
        "' called but not implemented. Call SetMethodOverride to define behavior.");
  }

 private:
  std::vector<std::string> m_call_history;
};

class MockDebugClient : public MockBase<IDebugClient> {
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

class MockDebugControl : public MockBase<IDebugControl> {
 public:
  STDMETHOD(Execute)(ULONG OutputControl, PCSTR Command, ULONG Flags) override {
    return MockMethod<HRESULT>("Execute", OutputControl, Command, Flags);
  }

  STDMETHOD(GetNumberBreakpoints)(PULONG Number) override {
    return MockMethod<HRESULT>("GetNumberBreakpoints", Number);
  }

  STDMETHOD(GetBreakpointByIndex)(ULONG Index, PDEBUG_BREAKPOINT* Bp) override {
    return MockMethod<HRESULT>("GetBreakpointByIndex", Index, Bp);
  }

  STDMETHOD(Evaluate)(PCSTR Expression,
                      ULONG DesiredType,
                      PDEBUG_VALUE Value,
                      PULONG RemainderIndex) override {
    return MockMethod<HRESULT>("Evaluate", Expression, DesiredType, Value,
                               RemainderIndex);
  }

  STDMETHOD(GetInterrupt)() override {
    return MockMethod<HRESULT>("GetInterrupt");
  }

  STDMETHOD(SetInterrupt)(ULONG Flags) override {
    return MockMethod<HRESULT>("SetInterrupt", Flags);
  }

  STDMETHOD(GetInterruptTimeout)(PULONG Seconds) override {
    return MockMethod<HRESULT>("GetInterruptTimeout", Seconds);
  }

  STDMETHOD(SetInterruptTimeout)(ULONG Seconds) override {
    return MockMethod<HRESULT>("SetInterruptTimeout", Seconds);
  }

  STDMETHOD(GetLogFile)(PSTR Buffer,
                        ULONG BufferSize,
                        PULONG FileSize,
                        PBOOL Append) override {
    return MockMethod<HRESULT>("GetLogFile", Buffer, BufferSize, FileSize,
                               Append);
  }

  STDMETHOD(OpenLogFile)(PCSTR File, BOOL Append) override {
    return MockMethod<HRESULT>("OpenLogFile", File, Append);
  }

  STDMETHOD(CloseLogFile)() override {
    return MockMethod<HRESULT>("CloseLogFile");
  }

  STDMETHOD(GetLogMask)(PULONG Mask) override {
    return MockMethod<HRESULT>("GetLogMask", Mask);
  }

  STDMETHOD(SetLogMask)(ULONG Mask) override {
    return MockMethod<HRESULT>("SetLogMask", Mask);
  }

  STDMETHOD(Input)(PSTR Buffer, ULONG BufferSize, PULONG InputSize) override {
    return MockMethod<HRESULT>("Input", Buffer, BufferSize, InputSize);
  }

  STDMETHOD(ReturnInput)(PCSTR Buffer) override {
    return MockMethod<HRESULT>("ReturnInput", Buffer);
  }

  STDMETHOD(Output)(ULONG Mask, PCSTR Format, ...) override {
    if (auto* func = GetOverride<std::function<HRESULT(ULONG, PCSTR, va_list)>>(
            "Output")) {
      va_list args;
      va_start(args, Format);
      HRESULT hr = (*func)(Mask, Format, args);
      va_end(args);
      return hr;
    }
    return E_NOTIMPL;
  }

  STDMETHOD(OutputVaList)(ULONG Mask, PCSTR Format, va_list Args) override {
    if (auto* func = GetOverride<std::function<HRESULT(ULONG, PCSTR, va_list)>>(
            "OutputVaList")) {
      return (*func)(Mask, Format, Args);
    }
    return E_NOTIMPL;
  }

  STDMETHOD(ControlledOutput)(ULONG OutputControl,
                              ULONG Mask,
                              PCSTR Format,
                              ...) override {
    if (auto* func =
            GetOverride<std::function<HRESULT(ULONG, ULONG, PCSTR, va_list)>>(
                "ControlledOutput")) {
      va_list args;
      va_start(args, Format);
      HRESULT hr = (*func)(OutputControl, Mask, Format, args);
      va_end(args);
      return hr;
    }
    return E_NOTIMPL;
  }

  STDMETHOD(ControlledOutputVaList)(ULONG OutputControl,
                                    ULONG Mask,
                                    PCSTR Format,
                                    va_list Args) override {
    if (auto* func =
            GetOverride<std::function<HRESULT(ULONG, ULONG, PCSTR, va_list)>>(
                "ControlledOutputVaList")) {
      return (*func)(OutputControl, Mask, Format, Args);
    }
    return E_NOTIMPL;
  }

  STDMETHOD(OutputPrompt)(ULONG OutputControl, PCSTR Format, ...) override {
    if (auto* func = GetOverride<std::function<HRESULT(ULONG, PCSTR, va_list)>>(
            "OutputPrompt")) {
      va_list args;
      va_start(args, Format);
      HRESULT hr = (*func)(OutputControl, Format, args);
      va_end(args);
      return hr;
    }
    return E_NOTIMPL;
  }

  STDMETHOD(OutputPromptVaList)(ULONG OutputControl,
                                PCSTR Format,
                                va_list Args) override {
    if (auto* func = GetOverride<std::function<HRESULT(ULONG, PCSTR, va_list)>>(
            "OutputPromptVaList")) {
      return (*func)(OutputControl, Format, Args);
    }
    return E_NOTIMPL;
  }

  STDMETHOD(GetPromptText)(PSTR Buffer,
                           ULONG BufferSize,
                           PULONG TextSize) override {
    return MockMethod<HRESULT>("GetPromptText", Buffer, BufferSize, TextSize);
  }

  STDMETHOD(OutputCurrentState)(ULONG OutputControl, ULONG Flags) override {
    return MockMethod<HRESULT>("OutputCurrentState", OutputControl, Flags);
  }

  STDMETHOD(OutputVersionInformation)(ULONG OutputControl) override {
    return MockMethod<HRESULT>("OutputVersionInformation", OutputControl);
  }

  STDMETHOD(GetNotifyEventHandle)(PULONG64 Handle) override {
    return MockMethod<HRESULT>("GetNotifyEventHandle", Handle);
  }

  STDMETHOD(SetNotifyEventHandle)(ULONG64 Handle) override {
    return MockMethod<HRESULT>("SetNotifyEventHandle", Handle);
  }

  STDMETHOD(Assemble)(ULONG64 Offset,
                      PCSTR Instr,
                      PULONG64 EndOffset) override {
    return MockMethod<HRESULT>("Assemble", Offset, Instr, EndOffset);
  }

  STDMETHOD(Disassemble)(ULONG64 Offset,
                         ULONG Flags,
                         PSTR Buffer,
                         ULONG BufferSize,
                         PULONG DisassemblySize,
                         PULONG64 EndOffset) override {
    return MockMethod<HRESULT>("Disassemble", Offset, Flags, Buffer, BufferSize,
                               DisassemblySize, EndOffset);
  }

  STDMETHOD(GetDisassembleEffectiveOffset)(PULONG64 Offset) override {
    return MockMethod<HRESULT>("GetDisassembleEffectiveOffset", Offset);
  }

  STDMETHOD(OutputDisassembly)(ULONG OutputControl,
                               ULONG64 Offset,
                               ULONG Flags,
                               PULONG64 EndOffset) override {
    return MockMethod<HRESULT>("OutputDisassembly", OutputControl, Offset,
                               Flags, EndOffset);
  }

  STDMETHOD(OutputDisassemblyLines)(ULONG OutputControl,
                                    ULONG PreviousLines,
                                    ULONG TotalLines,
                                    ULONG64 Offset,
                                    ULONG Flags,
                                    PULONG OffsetLine,
                                    PULONG64 StartOffset,
                                    PULONG64 EndOffset,
                                    PULONG64 LineOffsets) override {
    return MockMethod<HRESULT>("OutputDisassemblyLines", OutputControl,
                               PreviousLines, TotalLines, Offset, Flags,
                               OffsetLine, StartOffset, EndOffset, LineOffsets);
  }

  STDMETHOD(GetNearInstruction)(ULONG64 Offset,
                                LONG Delta,
                                PULONG64 NearOffset) override {
    return MockMethod<HRESULT>("GetNearInstruction", Offset, Delta, NearOffset);
  }

  STDMETHOD(GetStackTrace)(ULONG64 FrameOffset,
                           ULONG64 StackOffset,
                           ULONG64 InstructionOffset,
                           PDEBUG_STACK_FRAME Frames,
                           ULONG FramesSize,
                           PULONG FramesFilled) override {
    return MockMethod<HRESULT>("GetStackTrace", FrameOffset, StackOffset,
                               InstructionOffset, Frames, FramesSize,
                               FramesFilled);
  }

  STDMETHOD(GetReturnOffset)(PULONG64 Offset) override {
    return MockMethod<HRESULT>("GetReturnOffset", Offset);
  }

  STDMETHOD(OutputStackTrace)(ULONG OutputControl,
                              PDEBUG_STACK_FRAME Frames,
                              ULONG FramesSize,
                              ULONG Flags) override {
    return MockMethod<HRESULT>("OutputStackTrace", OutputControl, Frames,
                               FramesSize, Flags);
  }

  STDMETHOD(GetDebuggeeType)(PULONG Class, PULONG Qualifier) override {
    return MockMethod<HRESULT>("GetDebuggeeType", Class, Qualifier);
  }

  STDMETHOD(GetActualProcessorType)(PULONG Type) override {
    return MockMethod<HRESULT>("GetActualProcessorType", Type);
  }

  STDMETHOD(GetExecutingProcessorType)(PULONG Type) override {
    return MockMethod<HRESULT>("GetExecutingProcessorType", Type);
  }

  STDMETHOD(GetNumberPossibleExecutingProcessorTypes)(PULONG Number) override {
    return MockMethod<HRESULT>("GetNumberPossibleExecutingProcessorTypes",
                               Number);
  }

  STDMETHOD(GetPossibleExecutingProcessorTypes)(ULONG Start,
                                                ULONG Count,
                                                PULONG Types) override {
    return MockMethod<HRESULT>("GetPossibleExecutingProcessorTypes", Start,
                               Count, Types);
  }

  STDMETHOD(GetNumberProcessors)(PULONG Number) override {
    return MockMethod<HRESULT>("GetNumberProcessors", Number);
  }

  STDMETHOD(GetSystemVersion)(PULONG PlatformId,
                              PULONG Major,
                              PULONG Minor,
                              PSTR ServicePackString,
                              ULONG ServicePackStringSize,
                              PULONG ServicePackStringUsed,
                              PULONG ServicePackNumber,
                              PSTR BuildString,
                              ULONG BuildStringSize,
                              PULONG BuildStringUsed) override {
    return MockMethod<HRESULT>("GetSystemVersion", PlatformId, Major, Minor,
                               ServicePackString, ServicePackStringSize,
                               ServicePackStringUsed, ServicePackNumber,
                               BuildString, BuildStringSize, BuildStringUsed);
  }

  STDMETHOD(GetPageSize)(PULONG Size) override {
    return MockMethod<HRESULT>("GetPageSize", Size);
  }

  STDMETHOD(IsPointer64Bit)() override {
    return MockMethod<HRESULT>("IsPointer64Bit");
  }

  STDMETHOD(ReadBugCheckData)(PULONG Code,
                              PULONG64 Arg1,
                              PULONG64 Arg2,
                              PULONG64 Arg3,
                              PULONG64 Arg4) override {
    return MockMethod<HRESULT>("ReadBugCheckData", Code, Arg1, Arg2, Arg3,
                               Arg4);
  }

  STDMETHOD(GetNumberSupportedProcessorTypes)(PULONG Number) override {
    return MockMethod<HRESULT>("GetNumberSupportedProcessorTypes", Number);
  }

  STDMETHOD(GetSupportedProcessorTypes)(ULONG Start,
                                        ULONG Count,
                                        PULONG Types) override {
    return MockMethod<HRESULT>("GetSupportedProcessorTypes", Start, Count,
                               Types);
  }

  STDMETHOD(GetProcessorTypeNames)(ULONG Type,
                                   PSTR FullNameBuffer,
                                   ULONG FullNameBufferSize,
                                   PULONG FullNameSize,
                                   PSTR AbbrevNameBuffer,
                                   ULONG AbbrevNameBufferSize,
                                   PULONG AbbrevNameSize) override {
    return MockMethod<HRESULT>(
        "GetProcessorTypeNames", Type, FullNameBuffer, FullNameBufferSize,
        FullNameSize, AbbrevNameBuffer, AbbrevNameBufferSize, AbbrevNameSize);
  }

  STDMETHOD(GetEffectiveProcessorType)(PULONG Type) override {
    return MockMethod<HRESULT>("GetEffectiveProcessorType", Type);
  }

  STDMETHOD(SetEffectiveProcessorType)(ULONG Type) override {
    return MockMethod<HRESULT>("SetEffectiveProcessorType", Type);
  }

  STDMETHOD(GetExecutionStatus)(PULONG Status) override {
    return MockMethod<HRESULT>("GetExecutionStatus", Status);
  }

  STDMETHOD(SetExecutionStatus)(ULONG Status) override {
    return MockMethod<HRESULT>("SetExecutionStatus", Status);
  }

  STDMETHOD(GetCodeLevel)(PULONG Level) override {
    return MockMethod<HRESULT>("GetCodeLevel", Level);
  }

  STDMETHOD(SetCodeLevel)(ULONG Level) override {
    return MockMethod<HRESULT>("SetCodeLevel", Level);
  }

  STDMETHOD(GetEngineOptions)(PULONG Options) override {
    return MockMethod<HRESULT>("GetEngineOptions", Options);
  }

  STDMETHOD(AddEngineOptions)(ULONG Options) override {
    return MockMethod<HRESULT>("AddEngineOptions", Options);
  }

  STDMETHOD(RemoveEngineOptions)(ULONG Options) override {
    return MockMethod<HRESULT>("RemoveEngineOptions", Options);
  }

  STDMETHOD(SetEngineOptions)(ULONG Options) override {
    return MockMethod<HRESULT>("SetEngineOptions", Options);
  }

  STDMETHOD(GetSystemErrorControl)(PULONG OutputLevel,
                                   PULONG BreakLevel) override {
    return MockMethod<HRESULT>("GetSystemErrorControl", OutputLevel,
                               BreakLevel);
  }

  STDMETHOD(SetSystemErrorControl)(ULONG OutputLevel,
                                   ULONG BreakLevel) override {
    return MockMethod<HRESULT>("SetSystemErrorControl", OutputLevel,
                               BreakLevel);
  }

  STDMETHOD(GetTextMacro)(ULONG Slot,
                          PSTR Buffer,
                          ULONG BufferSize,
                          PULONG MacroSize) override {
    return MockMethod<HRESULT>("GetTextMacro", Slot, Buffer, BufferSize,
                               MacroSize);
  }

  STDMETHOD(SetTextMacro)(ULONG Slot, PCSTR Macro) override {
    return MockMethod<HRESULT>("SetTextMacro", Slot, Macro);
  }

  STDMETHOD(GetRadix)(PULONG Radix) override {
    return MockMethod<HRESULT>("GetRadix", Radix);
  }

  STDMETHOD(SetRadix)(ULONG Radix) override {
    return MockMethod<HRESULT>("SetRadix", Radix);
  }

  STDMETHOD(CoerceValue)(PDEBUG_VALUE In,
                         ULONG OutType,
                         PDEBUG_VALUE Out) override {
    return MockMethod<HRESULT>("CoerceValue", In, OutType, Out);
  }

  STDMETHOD(ExecuteCommandFile)(ULONG OutputControl,
                                PCSTR CommandFile,
                                ULONG Flags) override {
    return MockMethod<HRESULT>("ExecuteCommandFile", OutputControl, CommandFile,
                               Flags);
  }

  STDMETHOD(GetBreakpointById)(ULONG Id, PDEBUG_BREAKPOINT* Bp) override {
    return MockMethod<HRESULT>("GetBreakpointById", Id, Bp);
  }

  STDMETHOD(GetBreakpointParameters)(
      ULONG Count,
      PULONG Ids,
      ULONG Start,
      PDEBUG_BREAKPOINT_PARAMETERS Params) override {
    return MockMethod<HRESULT>("GetBreakpointParameters", Count, Ids, Start,
                               Params);
  }

  STDMETHOD(AddBreakpoint)(ULONG Type,
                           ULONG DesiredId,
                           PDEBUG_BREAKPOINT* Bp) override {
    return MockMethod<HRESULT>("AddBreakpoint", Type, DesiredId, Bp);
  }

  STDMETHOD(RemoveBreakpoint)(PDEBUG_BREAKPOINT Bp) override {
    return MockMethod<HRESULT>("RemoveBreakpoint", Bp);
  }

  STDMETHOD(AddExtension)(PCSTR Path, ULONG Flags, PULONG64 Handle) override {
    return MockMethod<HRESULT>("AddExtension", Path, Flags, Handle);
  }

  STDMETHOD(RemoveExtension)(ULONG64 Handle) override {
    return MockMethod<HRESULT>("RemoveExtension", Handle);
  }

  STDMETHOD(GetExtensionByPath)(PCSTR Path, PULONG64 Handle) override {
    return MockMethod<HRESULT>("GetExtensionByPath", Path, Handle);
  }

  STDMETHOD(CallExtension)(ULONG64 Handle,
                           PCSTR Function,
                           PCSTR Arguments) override {
    return MockMethod<HRESULT>("CallExtension", Handle, Function, Arguments);
  }

  STDMETHOD(GetExtensionFunction)(ULONG64 Handle,
                                  PCSTR FuncName,
                                  FARPROC* Function) override {
    return MockMethod<HRESULT>("GetExtensionFunction", Handle, FuncName,
                               Function);
  }

  STDMETHOD(GetWindbgExtensionApis32)(PWINDBG_EXTENSION_APIS32 Api) override {
    return MockMethod<HRESULT>("GetWindbgExtensionApis32", Api);
  }

  STDMETHOD(GetWindbgExtensionApis64)(PWINDBG_EXTENSION_APIS64 Api) override {
    return MockMethod<HRESULT>("GetWindbgExtensionApis64", Api);
  }

  STDMETHOD(GetNumberEventFilters)(PULONG SpecificEvents,
                                   PULONG SpecificExceptions,
                                   PULONG ArbitraryExceptions) override {
    return MockMethod<HRESULT>("GetNumberEventFilters", SpecificEvents,
                               SpecificExceptions, ArbitraryExceptions);
  }

  STDMETHOD(GetEventFilterText)(ULONG Index,
                                PSTR Buffer,
                                ULONG BufferSize,
                                PULONG TextSize) override {
    return MockMethod<HRESULT>("GetEventFilterText", Index, Buffer, BufferSize,
                               TextSize);
  }

  STDMETHOD(GetEventFilterCommand)(ULONG Index,
                                   PSTR Buffer,
                                   ULONG BufferSize,
                                   PULONG CommandSize) override {
    return MockMethod<HRESULT>("GetEventFilterCommand", Index, Buffer,
                               BufferSize, CommandSize);
  }

  STDMETHOD(SetEventFilterCommand)(ULONG Index, PCSTR Command) override {
    return MockMethod<HRESULT>("SetEventFilterCommand", Index, Command);
  }

  STDMETHOD(GetSpecificFilterParameters)(
      ULONG Start,
      ULONG Count,
      PDEBUG_SPECIFIC_FILTER_PARAMETERS Params) override {
    return MockMethod<HRESULT>("GetSpecificFilterParameters", Start, Count,
                               Params);
  }

  STDMETHOD(SetSpecificFilterParameters)(
      ULONG Start,
      ULONG Count,
      PDEBUG_SPECIFIC_FILTER_PARAMETERS Params) override {
    return MockMethod<HRESULT>("SetSpecificFilterParameters", Start, Count,
                               Params);
  }

  STDMETHOD(GetSpecificFilterArgument)(ULONG Index,
                                       PSTR Buffer,
                                       ULONG BufferSize,
                                       PULONG ArgumentSize) override {
    return MockMethod<HRESULT>("GetSpecificFilterArgument", Index, Buffer,
                               BufferSize, ArgumentSize);
  }

  STDMETHOD(SetSpecificFilterArgument)(ULONG Index, PCSTR Argument) override {
    return MockMethod<HRESULT>("SetSpecificFilterArgument", Index, Argument);
  }

  STDMETHOD(GetExceptionFilterParameters)(
      ULONG Count,
      PULONG Codes,
      ULONG Start,
      PDEBUG_EXCEPTION_FILTER_PARAMETERS Params) override {
    return MockMethod<HRESULT>("GetExceptionFilterParameters", Count, Codes,
                               Start, Params);
  }

  STDMETHOD(SetExceptionFilterParameters)(
      ULONG Count,
      PDEBUG_EXCEPTION_FILTER_PARAMETERS Params) override {
    return MockMethod<HRESULT>("SetExceptionFilterParameters", Count, Params);
  }

  STDMETHOD(GetExceptionFilterSecondCommand)(ULONG Index,
                                             PSTR Buffer,
                                             ULONG BufferSize,
                                             PULONG CommandSize) override {
    return MockMethod<HRESULT>("GetExceptionFilterSecondCommand", Index, Buffer,
                               BufferSize, CommandSize);
  }

  STDMETHOD(SetExceptionFilterSecondCommand)(ULONG Index,
                                             PCSTR Command) override {
    return MockMethod<HRESULT>("SetExceptionFilterSecondCommand", Index,
                               Command);
  }

  STDMETHOD(WaitForEvent)(ULONG Flags, ULONG Timeout) override {
    return MockMethod<HRESULT>("WaitForEvent", Flags, Timeout);
  }

  STDMETHOD(GetLastEventInformation)(PULONG Type,
                                     PULONG ProcessId,
                                     PULONG ThreadId,
                                     PVOID ExtraInformation,
                                     ULONG ExtraInformationSize,
                                     PULONG ExtraInformationUsed,
                                     PSTR Description,
                                     ULONG DescriptionSize,
                                     PULONG DescriptionUsed) override {
    return MockMethod<HRESULT>("GetLastEventInformation", Type, ProcessId,
                               ThreadId, ExtraInformation, ExtraInformationSize,
                               ExtraInformationUsed, Description,
                               DescriptionSize, DescriptionUsed);
  }

  STDMETHOD(CoerceValues)(ULONG Count,
                          PDEBUG_VALUE Values,
                          PULONG OutTypes,
                          PDEBUG_VALUE OutValues) override {
    return MockMethod<HRESULT>("CoerceValues", Count, Values, OutTypes,
                               OutValues);
  }
};

class MockDebugSymbols : public MockBase<IDebugSymbols> {
 public:
  STDMETHOD(GetScope)(PULONG64 InstructionOffset,
                      PDEBUG_STACK_FRAME ScopeFrame,
                      PVOID ScopeContext,
                      ULONG ScopeContextSize) override {
    return MockMethod<HRESULT>("GetScope", InstructionOffset, ScopeFrame,
                               ScopeContext, ScopeContextSize);
  }

  STDMETHOD(GetLineByOffset)(ULONG64 Offset,
                             PULONG Line,
                             PSTR FileBuffer,
                             ULONG FileBufferSize,
                             PULONG FileSize,
                             PULONG64 Displacement) override {
    return MockMethod<HRESULT>("GetLineByOffset", Offset, Line, FileBuffer,
                               FileBufferSize, FileSize, Displacement);
  }

  STDMETHOD(GetModuleByOffset)(ULONG64 Offset,
                               ULONG StartIndex,
                               PULONG Index,
                               PULONG64 Base) override {
    return MockMethod<HRESULT>("GetModuleByOffset", Offset, StartIndex, Index,
                               Base);
  }

  STDMETHOD(GetModuleNames)(ULONG Index,
                            ULONG64 Base,
                            PSTR ImageNameBuffer,
                            ULONG ImageNameBufferSize,
                            PULONG ImageNameSize,
                            PSTR ModuleNameBuffer,
                            ULONG ModuleNameBufferSize,
                            PULONG ModuleNameSize,
                            PSTR LoadedImageNameBuffer,
                            ULONG LoadedImageNameBufferSize,
                            PULONG LoadedImageNameSize) override {
    return MockMethod<HRESULT>(
        "GetModuleNames", Index, Base, ImageNameBuffer, ImageNameBufferSize,
        ImageNameSize, ModuleNameBuffer, ModuleNameBufferSize, ModuleNameSize,
        LoadedImageNameBuffer, LoadedImageNameBufferSize, LoadedImageNameSize);
  }

  STDMETHOD(GetSymbolOptions)(PULONG Options) override {
    return MockMethod<HRESULT>("GetSymbolOptions", Options);
  }

  STDMETHOD(AddSymbolOptions)(ULONG Options) override {
    return MockMethod<HRESULT>("AddSymbolOptions", Options);
  }

  STDMETHOD(RemoveSymbolOptions)(ULONG Options) override {
    return MockMethod<HRESULT>("RemoveSymbolOptions", Options);
  }

  STDMETHOD(SetSymbolOptions)(ULONG Options) override {
    return MockMethod<HRESULT>("SetSymbolOptions", Options);
  }

  STDMETHOD(GetNameByOffset)(ULONG64 Offset,
                             PSTR NameBuffer,
                             ULONG NameBufferSize,
                             PULONG NameSize,
                             PULONG64 Displacement) override {
    return MockMethod<HRESULT>("GetNameByOffset", Offset, NameBuffer,
                               NameBufferSize, NameSize, Displacement);
  }

  STDMETHOD(GetOffsetByName)(PCSTR Symbol, PULONG64 Offset) override {
    return MockMethod<HRESULT>("GetOffsetByName", Symbol, Offset);
  }

  STDMETHOD(GetNearNameByOffset)(ULONG64 Offset,
                                 LONG Delta,
                                 PSTR NameBuffer,
                                 ULONG NameBufferSize,
                                 PULONG NameSize,
                                 PULONG64 Displacement) override {
    return MockMethod<HRESULT>("GetNearNameByOffset", Offset, Delta, NameBuffer,
                               NameBufferSize, NameSize, Displacement);
  }

  STDMETHOD(GetOffsetByLine)(ULONG Line, PCSTR File, PULONG64 Offset) override {
    return MockMethod<HRESULT>("GetOffsetByLine", Line, File, Offset);
  }

  STDMETHOD(GetNumberModules)(PULONG Loaded, PULONG Unloaded) override {
    return MockMethod<HRESULT>("GetNumberModules", Loaded, Unloaded);
  }

  STDMETHOD(GetModuleByIndex)(ULONG Index, PULONG64 Base) override {
    return MockMethod<HRESULT>("GetModuleByIndex", Index, Base);
  }

  STDMETHOD(GetModuleByModuleName)(PCSTR Name,
                                   ULONG StartIndex,
                                   PULONG Index,
                                   PULONG64 Base) override {
    return MockMethod<HRESULT>("GetModuleByModuleName", Name, StartIndex, Index,
                               Base);
  }

  STDMETHOD(GetModuleParameters)(ULONG Count,
                                 PULONG64 Bases,
                                 ULONG Start,
                                 PDEBUG_MODULE_PARAMETERS Params) override {
    return MockMethod<HRESULT>("GetModuleParameters", Count, Bases, Start,
                               Params);
  }

  STDMETHOD(GetSymbolModule)(PCSTR Symbol, PULONG64 Base) override {
    return MockMethod<HRESULT>("GetSymbolModule", Symbol, Base);
  }

  STDMETHOD(GetTypeName)(ULONG64 Module,
                         ULONG TypeId,
                         PSTR NameBuffer,
                         ULONG NameBufferSize,
                         PULONG NameSize) override {
    return MockMethod<HRESULT>("GetTypeName", Module, TypeId, NameBuffer,
                               NameBufferSize, NameSize);
  }

  STDMETHOD(GetTypeId)(ULONG64 Module, PCSTR Name, PULONG TypeId) override {
    return MockMethod<HRESULT>("GetTypeId", Module, Name, TypeId);
  }

  STDMETHOD(GetTypeSize)(ULONG64 Module, ULONG TypeId, PULONG Size) override {
    return MockMethod<HRESULT>("GetTypeSize", Module, TypeId, Size);
  }

  STDMETHOD(GetFieldOffset)(ULONG64 Module,
                            ULONG TypeId,
                            PCSTR Field,
                            PULONG Offset) override {
    return MockMethod<HRESULT>("GetFieldOffset", Module, TypeId, Field, Offset);
  }

  STDMETHOD(GetSymbolTypeId)(PCSTR Symbol,
                             PULONG TypeId,
                             PULONG64 Module) override {
    return MockMethod<HRESULT>("GetSymbolTypeId", Symbol, TypeId, Module);
  }

  STDMETHOD(GetOffsetTypeId)(ULONG64 Offset,
                             PULONG TypeId,
                             PULONG64 Module) override {
    return MockMethod<HRESULT>("GetOffsetTypeId", Offset, TypeId, Module);
  }

  STDMETHOD(ReadTypedDataVirtual)(ULONG64 Offset,
                                  ULONG64 Module,
                                  ULONG TypeId,
                                  PVOID Buffer,
                                  ULONG BufferSize,
                                  PULONG BytesRead) override {
    return MockMethod<HRESULT>("ReadTypedDataVirtual", Offset, Module, TypeId,
                               Buffer, BufferSize, BytesRead);
  }

  STDMETHOD(WriteTypedDataVirtual)(ULONG64 Offset,
                                   ULONG64 Module,
                                   ULONG TypeId,
                                   PVOID Buffer,
                                   ULONG BufferSize,
                                   PULONG BytesWritten) override {
    return MockMethod<HRESULT>("WriteTypedDataVirtual", Offset, Module, TypeId,
                               Buffer, BufferSize, BytesWritten);
  }

  STDMETHOD(OutputTypedDataVirtual)(ULONG OutputControl,
                                    ULONG64 Offset,
                                    ULONG64 Module,
                                    ULONG TypeId,
                                    ULONG Flags) override {
    return MockMethod<HRESULT>("OutputTypedDataVirtual", OutputControl, Offset,
                               Module, TypeId, Flags);
  }

  STDMETHOD(ReadTypedDataPhysical)(ULONG64 Offset,
                                   ULONG64 Module,
                                   ULONG TypeId,
                                   PVOID Buffer,
                                   ULONG BufferSize,
                                   PULONG BytesRead) override {
    return MockMethod<HRESULT>("ReadTypedDataPhysical", Offset, Module, TypeId,
                               Buffer, BufferSize, BytesRead);
  }

  STDMETHOD(WriteTypedDataPhysical)(ULONG64 Offset,
                                    ULONG64 Module,
                                    ULONG TypeId,
                                    PVOID Buffer,
                                    ULONG BufferSize,
                                    PULONG BytesWritten) override {
    return MockMethod<HRESULT>("WriteTypedDataPhysical", Offset, Module, TypeId,
                               Buffer, BufferSize, BytesWritten);
  }

  STDMETHOD(OutputTypedDataPhysical)(ULONG OutputControl,
                                     ULONG64 Offset,
                                     ULONG64 Module,
                                     ULONG TypeId,
                                     ULONG Flags) override {
    return MockMethod<HRESULT>("OutputTypedDataPhysical", OutputControl, Offset,
                               Module, TypeId, Flags);
  }

  STDMETHOD(GetScopeSymbolGroup)(ULONG Flags,
                                 PDEBUG_SYMBOL_GROUP Update,
                                 PDEBUG_SYMBOL_GROUP* Symbols) override {
    return MockMethod<HRESULT>("GetScopeSymbolGroup", Flags, Update, Symbols);
  }

  STDMETHOD(CreateSymbolGroup)(PDEBUG_SYMBOL_GROUP* Group) override {
    return MockMethod<HRESULT>("CreateSymbolGroup", Group);
  }

  STDMETHOD(StartSymbolMatch)(PCSTR Pattern, PULONG64 Handle) override {
    return MockMethod<HRESULT>("StartSymbolMatch", Pattern, Handle);
  }

  STDMETHOD(GetNextSymbolMatch)(ULONG64 Handle,
                                PSTR Buffer,
                                ULONG BufferSize,
                                PULONG MatchSize,
                                PULONG64 Offset) override {
    return MockMethod<HRESULT>("GetNextSymbolMatch", Handle, Buffer, BufferSize,
                               MatchSize, Offset);
  }

  STDMETHOD(EndSymbolMatch)(ULONG64 Handle) override {
    return MockMethod<HRESULT>("EndSymbolMatch", Handle);
  }

  STDMETHOD(Reload)(PCSTR Module) override {
    return MockMethod<HRESULT>("Reload", Module);
  }

  STDMETHOD(GetSymbolPath)(PSTR Buffer,
                           ULONG BufferSize,
                           PULONG PathSize) override {
    return MockMethod<HRESULT>("GetSymbolPath", Buffer, BufferSize, PathSize);
  }

  STDMETHOD(SetSymbolPath)(PCSTR Path) override {
    return MockMethod<HRESULT>("SetSymbolPath", Path);
  }

  STDMETHOD(AppendSymbolPath)(PCSTR Addition) override {
    return MockMethod<HRESULT>("AppendSymbolPath", Addition);
  }

  STDMETHOD(GetImagePath)(PSTR Buffer,
                          ULONG BufferSize,
                          PULONG PathSize) override {
    return MockMethod<HRESULT>("GetImagePath", Buffer, BufferSize, PathSize);
  }

  STDMETHOD(SetImagePath)(PCSTR Path) override {
    return MockMethod<HRESULT>("SetImagePath", Path);
  }

  STDMETHOD(AppendImagePath)(PCSTR Addition) override {
    return MockMethod<HRESULT>("AppendImagePath", Addition);
  }

  STDMETHOD(GetSourcePath)(PSTR Buffer,
                           ULONG BufferSize,
                           PULONG PathSize) override {
    return MockMethod<HRESULT>("GetSourcePath", Buffer, BufferSize, PathSize);
  }

  STDMETHOD(GetSourcePathElement)(ULONG Index,
                                  PSTR Buffer,
                                  ULONG BufferSize,
                                  PULONG ElementSize) override {
    return MockMethod<HRESULT>("GetSourcePathElement", Index, Buffer,
                               BufferSize, ElementSize);
  }

  STDMETHOD(SetSourcePath)(PCSTR Path) override {
    return MockMethod<HRESULT>("SetSourcePath", Path);
  }

  STDMETHOD(AppendSourcePath)(PCSTR Addition) override {
    return MockMethod<HRESULT>("AppendSourcePath", Addition);
  }

  STDMETHOD(FindSourceFile)(ULONG StartElement,
                            PCSTR File,
                            ULONG Flags,
                            PULONG FoundElement,
                            PSTR Buffer,
                            ULONG BufferSize,
                            PULONG FoundSize) override {
    return MockMethod<HRESULT>("FindSourceFile", StartElement, File, Flags,
                               FoundElement, Buffer, BufferSize, FoundSize);
  }

  STDMETHOD(GetSourceFileLineOffsets)(PCSTR File,
                                      PULONG64 Buffer,
                                      ULONG BufferLines,
                                      PULONG FileLines) override {
    return MockMethod<HRESULT>("GetSourceFileLineOffsets", File, Buffer,
                               BufferLines, FileLines);
  }

  STDMETHOD(SetScope)(ULONG64 InstructionOffset,
                      PDEBUG_STACK_FRAME ScopeFrame,
                      PVOID ScopeContext,
                      ULONG ScopeContextSize) override {
    return MockMethod<HRESULT>("SetScope", InstructionOffset, ScopeFrame,
                               ScopeContext, ScopeContextSize);
  }

  STDMETHOD(ResetScope)() override { return MockMethod<HRESULT>("ResetScope"); }
};

#endif  // MOCK_DEBUG_INTERFACES_H

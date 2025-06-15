// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#ifndef MOCK_DEBUG_CONTROL_H
#define MOCK_DEBUG_CONTROL_H

#include "mock_debug_interface_base.h"

class MockDebugControl : public MockDebugInterfaceBase<IDebugControl> {
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

#endif  // MOCK_DEBUG_CONTROL_H

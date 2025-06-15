// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#ifndef MOCK_DEBUG_SYMBOLS_H
#define MOCK_DEBUG_SYMBOLS_H

#include "mock_debug_interface_base.h"

class MockDebugSymbols : public MockDebugInterfaceBase<IDebugSymbols> {
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

#endif  // MOCK_DEBUG_SYMBOLS_H

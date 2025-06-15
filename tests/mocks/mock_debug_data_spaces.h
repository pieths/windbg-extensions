// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#ifndef MOCK_DEBUG_DATA_SPACES_H
#define MOCK_DEBUG_DATA_SPACES_H

#include "mock_debug_interface_base.h"

class MockDebugDataSpaces : public MockDebugInterfaceBase<IDebugDataSpaces4> {
 public:
  // IDebugDataSpaces methods
  STDMETHOD(ReadVirtual)(ULONG64 Offset,
                         PVOID Buffer,
                         ULONG BufferSize,
                         PULONG BytesRead) override {
    return MockMethod<HRESULT>("ReadVirtual", Offset, Buffer, BufferSize,
                               BytesRead);
  }

  STDMETHOD(WriteVirtual)(ULONG64 Offset,
                          PVOID Buffer,
                          ULONG BufferSize,
                          PULONG BytesWritten) override {
    return MockMethod<HRESULT>("WriteVirtual", Offset, Buffer, BufferSize,
                               BytesWritten);
  }

  STDMETHOD(SearchVirtual)(ULONG64 Offset,
                           ULONG64 Length,
                           PVOID Pattern,
                           ULONG PatternSize,
                           ULONG PatternGranularity,
                           PULONG64 MatchOffset) override {
    return MockMethod<HRESULT>("SearchVirtual", Offset, Length, Pattern,
                               PatternSize, PatternGranularity, MatchOffset);
  }

  STDMETHOD(ReadVirtualUncached)(ULONG64 Offset,
                                 PVOID Buffer,
                                 ULONG BufferSize,
                                 PULONG BytesRead) override {
    return MockMethod<HRESULT>("ReadVirtualUncached", Offset, Buffer,
                               BufferSize, BytesRead);
  }

  STDMETHOD(WriteVirtualUncached)(ULONG64 Offset,
                                  PVOID Buffer,
                                  ULONG BufferSize,
                                  PULONG BytesWritten) override {
    return MockMethod<HRESULT>("WriteVirtualUncached", Offset, Buffer,
                               BufferSize, BytesWritten);
  }

  STDMETHOD(ReadPointersVirtual)(ULONG Count,
                                 ULONG64 Offset,
                                 PULONG64 Ptrs) override {
    return MockMethod<HRESULT>("ReadPointersVirtual", Count, Offset, Ptrs);
  }

  STDMETHOD(WritePointersVirtual)(ULONG Count,
                                  ULONG64 Offset,
                                  PULONG64 Ptrs) override {
    return MockMethod<HRESULT>("WritePointersVirtual", Count, Offset, Ptrs);
  }

  STDMETHOD(ReadPhysical)(ULONG64 Offset,
                          PVOID Buffer,
                          ULONG BufferSize,
                          PULONG BytesRead) override {
    return MockMethod<HRESULT>("ReadPhysical", Offset, Buffer, BufferSize,
                               BytesRead);
  }

  STDMETHOD(WritePhysical)(ULONG64 Offset,
                           PVOID Buffer,
                           ULONG BufferSize,
                           PULONG BytesWritten) override {
    return MockMethod<HRESULT>("WritePhysical", Offset, Buffer, BufferSize,
                               BytesWritten);
  }

  STDMETHOD(ReadControl)(ULONG Processor,
                         ULONG64 Offset,
                         PVOID Buffer,
                         ULONG BufferSize,
                         PULONG BytesRead) override {
    return MockMethod<HRESULT>("ReadControl", Processor, Offset, Buffer,
                               BufferSize, BytesRead);
  }

  STDMETHOD(WriteControl)(ULONG Processor,
                          ULONG64 Offset,
                          PVOID Buffer,
                          ULONG BufferSize,
                          PULONG BytesWritten) override {
    return MockMethod<HRESULT>("WriteControl", Processor, Offset, Buffer,
                               BufferSize, BytesWritten);
  }

  STDMETHOD(ReadIo)(ULONG InterfaceType,
                    ULONG BusNumber,
                    ULONG AddressSpace,
                    ULONG64 Offset,
                    PVOID Buffer,
                    ULONG BufferSize,
                    PULONG BytesRead) override {
    return MockMethod<HRESULT>("ReadIo", InterfaceType, BusNumber, AddressSpace,
                               Offset, Buffer, BufferSize, BytesRead);
  }

  STDMETHOD(WriteIo)(ULONG InterfaceType,
                     ULONG BusNumber,
                     ULONG AddressSpace,
                     ULONG64 Offset,
                     PVOID Buffer,
                     ULONG BufferSize,
                     PULONG BytesWritten) override {
    return MockMethod<HRESULT>("WriteIo", InterfaceType, BusNumber,
                               AddressSpace, Offset, Buffer, BufferSize,
                               BytesWritten);
  }

  STDMETHOD(ReadMsr)(ULONG Msr, PULONG64 Value) override {
    return MockMethod<HRESULT>("ReadMsr", Msr, Value);
  }

  STDMETHOD(WriteMsr)(ULONG Msr, ULONG64 Value) override {
    return MockMethod<HRESULT>("WriteMsr", Msr, Value);
  }

  STDMETHOD(ReadBusData)(ULONG BusDataType,
                         ULONG BusNumber,
                         ULONG SlotNumber,
                         ULONG Offset,
                         PVOID Buffer,
                         ULONG BufferSize,
                         PULONG BytesRead) override {
    return MockMethod<HRESULT>("ReadBusData", BusDataType, BusNumber,
                               SlotNumber, Offset, Buffer, BufferSize,
                               BytesRead);
  }

  STDMETHOD(WriteBusData)(ULONG BusDataType,
                          ULONG BusNumber,
                          ULONG SlotNumber,
                          ULONG Offset,
                          PVOID Buffer,
                          ULONG BufferSize,
                          PULONG BytesWritten) override {
    return MockMethod<HRESULT>("WriteBusData", BusDataType, BusNumber,
                               SlotNumber, Offset, Buffer, BufferSize,
                               BytesWritten);
  }

  STDMETHOD(CheckLowMemory)() override {
    return MockMethod<HRESULT>("CheckLowMemory");
  }

  STDMETHOD(ReadDebuggerData)(ULONG Index,
                              PVOID Buffer,
                              ULONG BufferSize,
                              PULONG DataSize) override {
    return MockMethod<HRESULT>("ReadDebuggerData", Index, Buffer, BufferSize,
                               DataSize);
  }

  STDMETHOD(ReadProcessorSystemData)(ULONG Processor,
                                     ULONG Index,
                                     PVOID Buffer,
                                     ULONG BufferSize,
                                     PULONG DataSize) override {
    return MockMethod<HRESULT>("ReadProcessorSystemData", Processor, Index,
                               Buffer, BufferSize, DataSize);
  }

  // IDebugDataSpaces2 methods
  STDMETHOD(VirtualToPhysical)(ULONG64 Virtual, PULONG64 Physical) override {
    return MockMethod<HRESULT>("VirtualToPhysical", Virtual, Physical);
  }

  STDMETHOD(GetVirtualTranslationPhysicalOffsets)(ULONG64 Virtual,
                                                  PULONG64 Offsets,
                                                  ULONG OffsetsSize,
                                                  PULONG Levels) override {
    return MockMethod<HRESULT>("GetVirtualTranslationPhysicalOffsets", Virtual,
                               Offsets, OffsetsSize, Levels);
  }

  STDMETHOD(ReadHandleData)(ULONG64 Handle,
                            ULONG DataType,
                            PVOID Buffer,
                            ULONG BufferSize,
                            PULONG DataSize) override {
    return MockMethod<HRESULT>("ReadHandleData", Handle, DataType, Buffer,
                               BufferSize, DataSize);
  }

  STDMETHOD(FillVirtual)(ULONG64 Start,
                         ULONG Size,
                         PVOID Pattern,
                         ULONG PatternSize,
                         PULONG Filled) override {
    return MockMethod<HRESULT>("FillVirtual", Start, Size, Pattern, PatternSize,
                               Filled);
  }

  STDMETHOD(FillPhysical)(ULONG64 Start,
                          ULONG Size,
                          PVOID Pattern,
                          ULONG PatternSize,
                          PULONG Filled) override {
    return MockMethod<HRESULT>("FillPhysical", Start, Size, Pattern,
                               PatternSize, Filled);
  }

  STDMETHOD(QueryVirtual)(ULONG64 Offset,
                          PMEMORY_BASIC_INFORMATION64 Info) override {
    return MockMethod<HRESULT>("QueryVirtual", Offset, Info);
  }

  // IDebugDataSpaces3 methods
  STDMETHOD(ReadImageNtHeaders)(ULONG64 ImageBase,
                                PIMAGE_NT_HEADERS64 Headers) override {
    return MockMethod<HRESULT>("ReadImageNtHeaders", ImageBase, Headers);
  }

  STDMETHOD(ReadTagged)(LPGUID Tag,
                        ULONG Offset,
                        PVOID Buffer,
                        ULONG BufferSize,
                        PULONG TotalSize) override {
    return MockMethod<HRESULT>("ReadTagged", Tag, Offset, Buffer, BufferSize,
                               TotalSize);
  }

  STDMETHOD(StartEnumTagged)(PULONG64 Handle) override {
    return MockMethod<HRESULT>("StartEnumTagged", Handle);
  }

  STDMETHOD(GetNextTagged)(ULONG64 Handle, LPGUID Tag, PULONG Size) override {
    return MockMethod<HRESULT>("GetNextTagged", Handle, Tag, Size);
  }

  STDMETHOD(EndEnumTagged)(ULONG64 Handle) override {
    return MockMethod<HRESULT>("EndEnumTagged", Handle);
  }

  // IDebugDataSpaces4 methods
  STDMETHOD(GetOffsetInformation)(ULONG Space,
                                  ULONG Which,
                                  ULONG64 Offset,
                                  PVOID Buffer,
                                  ULONG BufferSize,
                                  PULONG InfoSize) override {
    return MockMethod<HRESULT>("GetOffsetInformation", Space, Which, Offset,
                               Buffer, BufferSize, InfoSize);
  }

  STDMETHOD(GetNextDifferentlyValidOffsetVirtual)(
      ULONG64 Offset,
      PULONG64 NextOffset) override {
    return MockMethod<HRESULT>("GetNextDifferentlyValidOffsetVirtual", Offset,
                               NextOffset);
  }

  STDMETHOD(GetValidRegionVirtual)(ULONG64 Base,
                                   ULONG Size,
                                   PULONG64 ValidBase,
                                   PULONG ValidSize) override {
    return MockMethod<HRESULT>("GetValidRegionVirtual", Base, Size, ValidBase,
                               ValidSize);
  }

  STDMETHOD(SearchVirtual2)(ULONG64 Offset,
                            ULONG64 Length,
                            ULONG Flags,
                            PVOID Pattern,
                            ULONG PatternSize,
                            ULONG PatternGranularity,
                            PULONG64 MatchOffset) override {
    return MockMethod<HRESULT>("SearchVirtual2", Offset, Length, Flags, Pattern,
                               PatternSize, PatternGranularity, MatchOffset);
  }

  STDMETHOD(ReadMultiByteStringVirtual)(ULONG64 Offset,
                                        ULONG MaxBytes,
                                        PSTR Buffer,
                                        ULONG BufferSize,
                                        PULONG StringBytes) override {
    return MockMethod<HRESULT>("ReadMultiByteStringVirtual", Offset, MaxBytes,
                               Buffer, BufferSize, StringBytes);
  }

  STDMETHOD(ReadMultiByteStringVirtualWide)(ULONG64 Offset,
                                            ULONG MaxBytes,
                                            ULONG CodePage,
                                            PWSTR Buffer,
                                            ULONG BufferSize,
                                            PULONG StringBytes) override {
    return MockMethod<HRESULT>("ReadMultiByteStringVirtualWide", Offset,
                               MaxBytes, CodePage, Buffer, BufferSize,
                               StringBytes);
  }

  STDMETHOD(ReadUnicodeStringVirtual)(ULONG64 Offset,
                                      ULONG MaxBytes,
                                      ULONG CodePage,
                                      PSTR Buffer,
                                      ULONG BufferSize,
                                      PULONG StringBytes) override {
    return MockMethod<HRESULT>("ReadUnicodeStringVirtual", Offset, MaxBytes,
                               CodePage, Buffer, BufferSize, StringBytes);
  }

  STDMETHOD(ReadUnicodeStringVirtualWide)(ULONG64 Offset,
                                          ULONG MaxBytes,
                                          PWSTR Buffer,
                                          ULONG BufferSize,
                                          PULONG StringBytes) override {
    return MockMethod<HRESULT>("ReadUnicodeStringVirtualWide", Offset, MaxBytes,
                               Buffer, BufferSize, StringBytes);
  }

  STDMETHOD(ReadPhysical2)(ULONG64 Offset,
                           ULONG Flags,
                           PVOID Buffer,
                           ULONG BufferSize,
                           PULONG BytesRead) override {
    return MockMethod<HRESULT>("ReadPhysical2", Offset, Flags, Buffer,
                               BufferSize, BytesRead);
  }

  STDMETHOD(WritePhysical2)(ULONG64 Offset,
                            ULONG Flags,
                            PVOID Buffer,
                            ULONG BufferSize,
                            PULONG BytesWritten) override {
    return MockMethod<HRESULT>("WritePhysical2", Offset, Flags, Buffer,
                               BufferSize, BytesWritten);
  }
};

#endif  // MOCK_DEBUG_DATA_SPACES_H

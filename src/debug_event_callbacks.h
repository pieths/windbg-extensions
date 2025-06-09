// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#include <dbgeng.h>

// A convenience class which implements some of the
// required functionality so that it doesn't need to be
// implemented in every derived class.
class DebugEventCallbacks : public DebugBaseEventCallbacks {
 public:
  DebugEventCallbacks(ULONG interest_mask)
      : m_refCount(1), m_interest_mask(interest_mask) {}

  virtual ~DebugEventCallbacks() = default;

  STDMETHOD(QueryInterface)(REFIID InterfaceId, PVOID* Interface) override {
    *Interface = NULL;
    if (IsEqualIID(InterfaceId, __uuidof(IUnknown)) ||
        IsEqualIID(InterfaceId, __uuidof(IDebugEventCallbacks))) {
      *Interface = (IDebugEventCallbacks*)this;
      AddRef();
      return S_OK;
    }
    return E_NOINTERFACE;
  }

  STDMETHOD_(ULONG, AddRef)() override {
    return InterlockedIncrement(&m_refCount);
  }

  STDMETHOD_(ULONG, Release)() override {
    ULONG count = InterlockedDecrement(&m_refCount);
    if (count == 0) {
      delete this;
    }
    return count;
  }

  STDMETHOD(GetInterestMask)(PULONG mask) override {
    *mask = m_interest_mask;
    return S_OK;
  }

 private:
  LONG m_refCount;
  ULONG m_interest_mask;
};

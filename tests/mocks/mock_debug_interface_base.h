// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#ifndef MOCK_DEBUG_INTERFACE_BASE_H
#define MOCK_DEBUG_INTERFACE_BASE_H

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
class MockDebugInterfaceBase : public TInterface {
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

#endif  // MOCK_DEBUG_INTERFACE_BASE_H

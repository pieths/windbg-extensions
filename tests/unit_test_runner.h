// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#ifndef UNIT_TEST_RUNNER_H
#define UNIT_TEST_RUNNER_H

#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class UnitTestRunner {
 private:
  struct TestCase {
    std::string name;
    std::function<void()> test;
  };

  std::vector<TestCase> tests;
  int passed_ = 0;
  int failed_ = 0;

 public:
  void RegisterTest(const std::string& name, std::function<void()> test) {
    tests.push_back({name, test});
  }

  int RunTests() {
    std::cout << "Running " << tests.size() << " tests...\n\n";

    for (const auto& test : tests) {
      std::cout << "Running: " << test.name << " ... ";

      try {
        test.test();
        std::cout << "PASSED\n";
        passed_++;
      } catch (const std::exception& e) {
        std::cout << "FAILED\n";
        std::cout << "  Error: " << e.what() << "\n";
        failed_++;
      }
    }

    std::cout << "\n========================================\n";
    std::cout << "Test Results: " << passed_ << " passed, " << failed_
              << " failed\n";
    std::cout << "========================================\n";

    return failed_ > 0 ? 1 : 0;
  }
};

#define DECLARE_TEST_RUNNER()       \
  namespace {                       \
  UnitTestRunner& GetTestRunner() { \
    static UnitTestRunner runner;   \
    return runner;                  \
  }                                 \
  }

#define TEST(test_name)                                                     \
  static struct Test_##test_name##_##__LINE__ {                             \
    Test_##test_name##_##__LINE__() {                                       \
      GetTestRunner().RegisterTest(#test_name, [this]() { this->Test(); }); \
    }                                                                       \
    void Test();                                                            \
  } test_##test_name##_##__LINE__;                                          \
                                                                            \
  void Test_##test_name##_##__LINE__::Test()

#define RUN_ALL_TESTS() GetTestRunner().RunTests()

// Test assertion helpers
#define TEST_ASSERT(condition)                                 \
  if (!(condition)) {                                          \
    throw std::runtime_error("Assertion failed: " #condition); \
  }

#define TEST_ASSERT_EQUALS(expected, actual)                       \
  if ((expected) != (actual)) {                                    \
    std::stringstream ss;                                          \
    ss << "Expected: " << (expected) << ", but got: " << (actual); \
    throw std::runtime_error(ss.str());                            \
  }

#define TEST_ASSERT_STRING_CONTAINS(haystack, needle)                        \
  if (std::string(haystack).find(needle) == std::string::npos) {             \
    std::stringstream ss;                                                    \
    ss << "String '" << haystack << "' does not contain '" << needle << "'"; \
    throw std::runtime_error(ss.str());                                      \
  }

#endif  // UNIT_TEST_RUNNER_H

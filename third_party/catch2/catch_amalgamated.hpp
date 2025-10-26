// Minimal single-header testing framework compatible with a small subset of Catch2 API
// This is NOT full Catch2; it supports only TEST_CASE and REQUIRE used in this project.
// Provided to keep the project self-contained and offline.

#pragma once
#include <functional>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

namespace mini_catch2 {
struct TestCase { std::string name; std::function<void()> fn; };
inline std::vector<TestCase>& registry() { static std::vector<TestCase> r; return r; }
struct Registrar { Registrar(const std::string& n, std::function<void()> f) { registry().push_back({n, std::move(f)}); } };
inline int run_all() {
  int failed = 0;
  for (auto& t : registry()) {
    try {
      t.fn();
      std::cout << "[ OK ] " << t.name << "\n";
    } catch (const std::exception& e) {
      ++failed;
      std::cout << "[FAIL] " << t.name << ": " << e.what() << "\n";
    } catch (...) {
      ++failed;
      std::cout << "[FAIL] " << t.name << ": unknown exception\n";
    }
  }
  std::cout << "\n" << (registry().size() - failed) << "/" << registry().size() << " tests passed\n";
  return failed ? 1 : 0;
}

struct failure : public std::exception {
  std::string msg; failure(std::string m) : msg(std::move(m)) {}
  const char* what() const noexcept override { return msg.c_str(); }
};
} // namespace mini_catch2

#define TEST_CASE(name, tags) \
  static void CATCH2_UNIQUE_TEST_(); \
  static mini_catch2::Registrar CATCH2_UNIQUE_REGISTRAR_(name, CATCH2_UNIQUE_TEST_); \
  static void CATCH2_UNIQUE_TEST_()

#define CATCH2_CONCAT_(a,b) a##b
#define CATCH2_UNIQUE_TEST_ CATCH2_CONCAT_(_mini_catch_test_, __LINE__)
#define CATCH2_UNIQUE_REGISTRAR_ CATCH2_CONCAT_(_mini_catch_registrar_, __LINE__)

#define REQUIRE(expr) do { if(!(expr)) throw mini_catch2::failure(std::string("REQUIRE failed: ") + #expr + " at " + __FILE__ + ":" + std::to_string(__LINE__)); } while(0)

#ifdef CATCH_CONFIG_MAIN
int main() { return mini_catch2::run_all(); }
#endif

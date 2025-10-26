#pragma once
#include <cstdint>
#include <string>
#include <chrono>

namespace nhft {

// Build-time code hash from git short SHA or "unknown"
std::string code_hash();

// Best-effort CPU affinity pinning (Linux only); returns true on success
bool pin_to_cpu(int cpu);

// FNV-1a 64-bit checksum
uint64_t fnv1a64(const void* data, size_t len);
uint64_t fnv1a64_str(const std::string& s);

// Time helpers
using steady_clock = std::chrono::steady_clock;
using time_point = steady_clock::time_point;
inline uint64_t to_ns(time_point t) {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch()).count();
}
inline double ns_to_ms(uint64_t ns) { return double(ns) / 1e6; }

} // namespace nhft

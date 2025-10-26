#include "util.hpp"
#include <cstring>
#include <thread>
#include <vector>
#include <iostream>

#ifdef __linux__
#include <pthread.h>
#endif

namespace nhft {

std::string code_hash() {
#ifdef NANOHFT_CODE_HASH
  return NANOHFT_CODE_HASH;
#else
  return "unknown";
#endif
}

bool pin_to_cpu(int cpu) {
#ifdef __linux__
  if (cpu < 0) return false;
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);
  pthread_t thread = pthread_self();
  int rc = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
  if (rc != 0) {
    std::cerr << "[warn] pin_to_cpu failed: " << rc << "\n";
    return false;
  }
  return true;
#else
  (void)cpu;
  std::cerr << "[warn] pin_to_cpu not supported on this platform\n";
  return false;
#endif
}

uint64_t fnv1a64(const void* data, size_t len) {
  const uint8_t* p = static_cast<const uint8_t*>(data);
  uint64_t hash = 1469598103934665603ull; // FNV offset basis
  const uint64_t prime = 1099511628211ull;
  for (size_t i = 0; i < len; ++i) {
    hash ^= p[i];
    hash *= prime;
  }
  return hash;
}

uint64_t fnv1a64_str(const std::string& s) { return fnv1a64(s.data(), s.size()); }

} // namespace nhft

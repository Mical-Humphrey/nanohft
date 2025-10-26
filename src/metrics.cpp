#include "metrics.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <cmath>

#ifdef __linux__
#include <unistd.h>
#include <cstdio>
#endif

namespace nhft {

LatencyRecorder::LatencyRecorder(double max_ms, int bins, size_t sample_cap)
  : max_ms_(max_ms), bins_(bins), hist_(bins, 0), sample_cap_(sample_cap) {}

void LatencyRecorder::add_sample(double ms) {
  if (ms < 0) ms = 0;
  int idx = (int)std::min((double)(bins_-1), std::floor(ms / max_ms_ * bins_));
  hist_[idx]++;
  if (samples_.size() < sample_cap_) samples_.push_back(ms);
  if (ms > max_observed_) max_observed_ = ms;
}

Percentiles LatencyRecorder::percentiles() const {
  Percentiles p{};
  uint64_t total = 0; for (auto c : hist_) total += c;
  if (total == 0) { p.jitter_ratio = 0; return p; }
  auto kth = [&](double q){
    uint64_t k = (uint64_t)std::ceil(q * total);
    uint64_t acc = 0;
    for (int i=0;i<bins_;++i) { acc += hist_[i]; if (acc >= k) return (i+0.5)*(max_ms_/bins_); }
    return max_ms_;
  };
  p.p50 = kth(0.50);
  p.p95 = kth(0.95);
  p.p99 = kth(0.99);
  p.max = std::max(max_observed_, p.p99);
  p.jitter_ratio = (p.p50 > 0) ? (p.p99 / p.p50) : 0.0;
  return p;
}

std::string LatencyRecorder::csv_samples() const {
  std::ostringstream oss;
  for (size_t i=0;i<samples_.size();++i) {
    oss << std::fixed << std::setprecision(6) << samples_[i] << "\n";
  }
  return oss.str();
}

std::string Metrics::to_json() const {
  auto p = latency.percentiles();
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(3);
  oss << "{ \"version\": \"1\", \"fingerprint\": { "
      << "\"seed\": " << seed << ", \"code_hash\": \"" << code_hash << "\", \"symbols\": " << symbols
      << ", \"rate\": " << rate << ", \"mode\": \"" << mode << "\" }, ";
  oss << "\"latency_ms\": { \"p50\": " << p.p50 << ", \"p95\": " << p.p95 << ", \"p99\": " << p.p99
      << ", \"max\": " << p.max << ", \"jitter_ratio\": " << p.jitter_ratio << " }, ";
  oss << "\"throughput\": { \"eps\": " << eps << " }, ";
  oss << "\"reliability\": { \"drops\": " << reliability.drops << ", \"queue_depth_max\": " << reliability.queue_depth_max
      << ", \"idempotency_violations\": " << reliability.idempotency_violations << ", \"exposure_blocks\": " << reliability.exposure_blocks << " }, ";
  oss << "\"resources\": { \"rss_mb\": " << rss_mb << " } }";
  return oss.str();
}

double rss_mb() {
#ifdef __linux__
  // Read /proc/self/statm: size resident shared text lib data dt
  FILE* f = std::fopen("/proc/self/statm", "r");
  if (!f) return 0.0;
  long pages = 0, resident = 0;
  if (std::fscanf(f, "%ld %ld", &pages, &resident) != 2) { std::fclose(f); return 0.0; }
  std::fclose(f);
  long page_size = sysconf(_SC_PAGESIZE);
  return (double)resident * (double)page_size / (1024.0 * 1024.0);
#else
  return 0.0;
#endif
}

} // namespace nhft

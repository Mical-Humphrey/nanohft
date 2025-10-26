#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <sstream>

namespace nhft {

struct Percentiles { double p50=0, p95=0, p99=0, max=0, jitter_ratio=0; };

class LatencyRecorder {
public:
  // Histogram up to max_ms in 'bins' bins; also store up to sample_cap samples (ms)
  LatencyRecorder(double max_ms=5.0, int bins=64, size_t sample_cap=2000);
  void add_sample(double ms);
  Percentiles percentiles() const;
  std::string csv_samples_header() const { return "latency_ms"; }
  std::string csv_samples() const; // one value per line
private:
  double max_ms_;
  int bins_;
  std::vector<uint64_t> hist_;
  std::vector<double> samples_;
  size_t sample_cap_;
  double max_observed_ = 0.0;
};

struct ReliabilityCounters {
  uint64_t drops = 0;
  uint64_t queue_depth_max = 0;
  uint64_t idempotency_violations = 0;
  uint64_t exposure_blocks = 0;
};

struct Metrics {
  // fingerprint
  int seed = 7;
  std::string code_hash;
  int symbols = 4;
  int rate = 100000;
  std::string mode;

  // latency
  LatencyRecorder latency;
  // throughput
  double eps = 0.0;
  // reliability
  ReliabilityCounters reliability;
  // resources
  double rss_mb = 0.0; // Linux only

  std::string to_json() const;
};

// Best-effort resident set size in MB (Linux only); returns 0.0 on unsupported platforms.
double rss_mb();

} // namespace nhft

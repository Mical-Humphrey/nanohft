#pragma once
#include <cstdint>
#include <vector>
#include <random>
#include <string>

namespace nhft {

struct MdEvent {
  uint64_t ts_ns; // production timestamp
  int symbol;     // 0..S-1
  double mid;     // mid price
  double spread;  // spread
};

struct Burst { double t_s=0; double dur_s=0; double x=1; };

class MdFeed {
public:
  MdFeed(int symbols, int rate_eps, uint64_t seed, const std::vector<Burst>& bursts, bool deterministic_timing=false);
  // Calculate next scheduled ts (ns) and event; returns false if past end time in deterministic mode
  MdEvent next(double now_s);
  // per-symbol initial mid
  const std::vector<double>& initial_mids() const { return mids_; }
private:
  int S_;
  int rate_;
  uint64_t seed_;
  std::vector<Burst> bursts_;
  std::mt19937_64 rng_;
  std::vector<double> mids_;
  int sym_idx_ = -1; // for round-robin cycling per feed instance
};

} // namespace nhft

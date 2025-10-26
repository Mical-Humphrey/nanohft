#include "mdfeed.hpp"
#include <cmath>

namespace nhft {

MdFeed::MdFeed(int symbols, int rate_eps, uint64_t seed, const std::vector<Burst>& bursts, bool deterministic_timing)
  : S_(symbols), rate_(rate_eps), seed_(seed), bursts_(bursts), rng_(seed) {
  (void)deterministic_timing;
  mids_.resize(S_);
  for (int i=0;i<S_;++i) mids_[i] = 100.0 + i; // simple ladder of prices
}

static inline double rate_with_bursts(int base_rate, double t, const std::vector<Burst>& bursts) {
  double r = base_rate;
  for (auto& b : bursts) {
    if (t >= b.t_s && t < (b.t_s + b.dur_s)) r *= b.x;
  }
  return r;
}

MdEvent MdFeed::next(double now_s) {
  // Choose symbol round-robin; per-instance cycling for determinism
  sym_idx_ = (sym_idx_ + 1) % S_;
  // Random walk on mid
  std::uniform_real_distribution<double> u(-0.01, 0.01);
  mids_[sym_idx_] = std::max(0.01, mids_[sym_idx_] * (1.0 + u(rng_)));
  MdEvent ev{};
  ev.symbol = sym_idx_;
  ev.mid = mids_[sym_idx_];
  ev.spread = 0.01; // constant 1c for simplicity
  ev.ts_ns = (uint64_t)std::llround(now_s * 1e9);
  return ev;
}

} // namespace nhft

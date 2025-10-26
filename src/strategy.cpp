#include "strategy.hpp"
#include <cmath>

namespace nhft {

Strategy::Strategy(int symbols, double alpha, double z_entry)
  : S_(symbols), alpha_(alpha), z_entry_(z_entry), prev_mid_(symbols, 0.0), ewma_(symbols, 0.0), ewvar_(symbols, 1e-6) {}

Decision Strategy::on_mid(int sym, double mid) {
  double ret = 0.0;
  if (prev_mid_[sym] > 0) ret = (mid - prev_mid_[sym]) / prev_mid_[sym];
  prev_mid_[sym] = mid;
  // EWMA and EWVAR
  double d = ret - ewma_[sym];
  ewma_[sym] += alpha_ * d;
  ewvar_[sym] = (1 - alpha_) * (ewvar_[sym] + alpha_ * d * d);
  double z = (ewvar_[sym] > 1e-12) ? (ewma_[sym] / std::sqrt(ewvar_[sym])) : 0.0;
  Decision dec{};
  dec.reason_score = z;
  if (z <= -z_entry_) { dec.side = +1; dec.qty = 1.0; }
  else if (z >= +z_entry_) { dec.side = -1; dec.qty = 1.0; }
  else { dec.side = 0; dec.qty = 0.0; }
  return dec;
}

} // namespace nhft

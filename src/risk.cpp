#include "risk.hpp"
#include <cmath>

namespace nhft {

Risk::Risk(int symbols, double per_trade_notional_cap, double daily_loss_cap)
  : per_trade_cap_(per_trade_notional_cap), daily_loss_cap_(daily_loss_cap), position_(symbols, 0.0) {}

RiskResult Risk::check(int sym, int side, double qty, double px) {
  RiskResult r{};
  double notional = std::abs(qty * px);
  if (notional > per_trade_cap_) {
    r.allowed = false; r.reason = "per_trade_cap"; last_reason_ = r.reason; exposure_blocks_++; return r;
  }
  if (pnl_ <= -daily_loss_cap_) {
    r.allowed = false; r.reason = "daily_loss_cap"; last_reason_ = r.reason; exposure_blocks_++; return r;
  }
  (void)sym; (void)side; // further per-symbol risk could be added
  return r;
}

void Risk::on_fill(int sym, int side, double qty, double px) {
  // side: +1 buy, -1 sell
  position_[sym] += side * qty;
  // mark-to-market PnL effect: assume IOC incurs a meaningful cost to demonstrate caps
  pnl_ -= 0.01 * std::abs(qty) * px; // 1% notional cost per fill (teaching/demo value)
}

} // namespace nhft

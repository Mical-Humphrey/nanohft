#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace nhft {

struct RiskResult {
  bool allowed = true;
  std::string reason;
};

class Risk {
public:
  Risk(int symbols, double per_trade_notional_cap=10000.0, double daily_loss_cap=1000.0);
  RiskResult check(int sym, int side, double qty, double px);
  void on_fill(int sym, int side, double qty, double px);
  double pnl() const { return pnl_; }
  uint64_t exposure_blocks() const { return exposure_blocks_; }
  std::string last_reason() const { return last_reason_; }
private:
  double per_trade_cap_;
  double daily_loss_cap_;
  std::vector<double> position_;
  double pnl_ = 0.0;
  uint64_t exposure_blocks_ = 0;
  std::string last_reason_;
};

} // namespace nhft

#pragma once
#include <unordered_set>
#include <string>
#include <fstream>
#include <cstdint>

namespace nhft {

struct Trade {
  uint64_t ts_ns;
  int symbol;
  int side; // +1 buy, -1 sell
  double qty;
  double px;
  std::string reason;
};

class Router {
public:
  Router(uint64_t seed, const std::string& trades_csv_path);
  // Returns true if filled; idempotent order IDs; track duplicates
  bool ioc_fill(uint64_t order_id, uint64_t ts_ns, int sym, int side, double qty, double mid, double half_spread, const std::string& reason_ex);
  uint64_t idempotency_violations() const { return idem_violations_; }
private:
  std::unordered_set<uint64_t> seen_;
  std::ofstream out_;
  uint64_t seed_;
  uint64_t idem_violations_ = 0;
};

} // namespace nhft

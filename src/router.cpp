#include "router.hpp"
#include "util.hpp"
#include <iomanip>

namespace nhft {

Router::Router(uint64_t seed, const std::string& trades_csv_path) : seed_(seed) {
  out_.open(trades_csv_path);
  if (out_.is_open()) {
    out_ << "ts,symbol,side,qty,px,reason_excerpt\n";
  }
}

bool Router::ioc_fill(uint64_t order_id, uint64_t ts_ns, int sym, int side, double qty, double mid, double half_spread, const std::string& reason_ex) {
  // Track idempotency
  if (!seen_.insert(order_id).second) {
    ++idem_violations_;
    return false;
  }
  if (!out_.is_open()) return false;
  double px = mid + (side>0 ? +half_spread : -half_spread);
  out_ << ts_ns << "," << sym << "," << side << "," << std::fixed << std::setprecision(6) << qty << "," << px << "," << reason_ex << "\n";
  return true;
}

} // namespace nhft

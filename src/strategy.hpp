#pragma once
#include <vector>

namespace nhft {

struct Decision {
  // -1 sell, 0 hold, +1 buy
  int side = 0;
  double qty = 0; // units
  double reason_score = 0; // for logging excerpt
};

class Strategy {
public:
  Strategy(int symbols, double alpha=0.2, double z_entry=1.5);
  Decision on_mid(int sym, double mid);
private:
  int S_;
  double alpha_;
  double z_entry_;
  std::vector<double> prev_mid_;
  std::vector<double> ewma_;
  std::vector<double> ewvar_;
};

} // namespace nhft

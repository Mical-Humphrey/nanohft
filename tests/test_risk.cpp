#include <catch2/catch_amalgamated.hpp>
#include "risk.hpp"

using namespace nhft;

TEST_CASE("Risk gating triggers on caps", "[risk]") {
  Risk r(2, /*per_trade_cap*/ 10.0, /*daily_loss_cap*/ 1.0);
  // per-trade notional cap breach
  auto rr1 = r.check(0, +1, 2.0, 6.0); // notional=12 > 10
  REQUIRE(rr1.allowed == false);
  REQUIRE(r.exposure_blocks() >= 1);

  // allowed trade then accumulate losses
  auto rr2 = r.check(0, +1, 1.0, 5.0); // notional=5 <= 10
  REQUIRE(rr2.allowed == true);
  r.on_fill(0, +1, 1.0, 5.0);

  // Trigger daily loss cap by simulating more cost
  for (int i=0;i<200;++i) r.on_fill(0, +1, 1.0, 5.0);
  auto rr3 = r.check(1, -1, 0.5, 5.0);
  REQUIRE(rr3.allowed == false);
}

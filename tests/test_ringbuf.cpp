#define CATCH_CONFIG_MAIN
#include <catch2/catch_amalgamated.hpp>
#include "ringbuf.hpp"
#include <thread>
#include <atomic>

using nhft::SpscRing;

TEST_CASE("SPSC ring basic ordering and no data loss within capacity", "[ringbuf]") {
  struct P { int v; };
  SpscRing<P> rb(1u<<12);
  const int N = 200000; // keep CI fast
  std::atomic<int> produced{0}, consumed{0};
  std::atomic<bool> done{false};
  std::thread prod([&]{
    for (int i=0;i<N;++i) {
      P p{i};
      while (!rb.push(p)) { /* drop-on-full policy: here we block to avoid drops for test */ }
      produced++;
    }
    done.store(true);
  });
  std::thread cons([&]{
    int expected=0; P out{};
    while (!done.load() || consumed.load()<N) {
      if (rb.pop(out)) {
        REQUIRE(out.v == expected);
        expected++;
        consumed++;
      } else {
        std::this_thread::yield();
      }
    }
  });
  prod.join(); cons.join();
  REQUIRE(produced.load() == N);
  REQUIRE(consumed.load() == N);
  REQUIRE(rb.max_depth() > 0);
}

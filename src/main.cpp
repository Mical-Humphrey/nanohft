#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "util.hpp"
#include "ringbuf.hpp"
#include "metrics.hpp"
#include "mdfeed.hpp"
#include "strategy.hpp"
#include "risk.hpp"
#include "router.hpp"

using namespace std::chrono;

namespace nhft {

struct Args {
  int duration_s = 20;
  int rate = 100000;
  int symbols = 4;
  std::vector<Burst> bursts;
  std::string mode = "optimized"; // naive|optimized
  int seed = 7;
  std::optional<int> affinity;
  std::string report = "./out/run";
  bool determinism_check = false;
};

static bool parse_burst(const std::string& s, Burst& b) {
  // format t=10,dur=2,x=5
  double t=0, dur=0, x=1;
  if (sscanf(s.c_str(), "t=%lf,dur=%lf,x=%lf", &t, &dur, &x) == 3) { b.t_s=t; b.dur_s=dur; b.x=x; return true; }
  return false;
}

static Args parse_args(int argc, char** argv) {
  Args a;
  for (int i=1;i<argc;++i) {
    std::string arg = argv[i];
    auto next = [&]{ return (i+1<argc)? std::string(argv[++i]) : std::string(); };
    if (arg == "--duration-s") a.duration_s = std::stoi(next());
    else if (arg == "--rate") a.rate = std::stoi(next());
    else if (arg == "--symbols") a.symbols = std::stoi(next());
    else if (arg == "--burst") { Burst b; if (parse_burst(next(), b)) a.bursts.push_back(b); }
    else if (arg == "--mode") a.mode = next();
    else if (arg == "--seed") a.seed = std::stoi(next());
    else if (arg == "--affinity") { int c = std::stoi(next()); a.affinity = c; }
    else if (arg == "--report") a.report = next();
    else if (arg == "--determinism-check") a.determinism_check = true;
  }
  return a;
}

struct EngineResult {
  Metrics metrics;
  std::string metrics_json;
};

struct OrderKey { uint64_t seed; int sym; uint64_t seq; int side; };
static uint64_t make_order_id(const OrderKey& k) {
  uint64_t x = 0;
  x ^= nhft::fnv1a64(&k.seed, sizeof(k.seed));
  x ^= nhft::fnv1a64(&k.sym, sizeof(k.sym));
  x ^= nhft::fnv1a64(&k.seq, sizeof(k.seq));
  x ^= nhft::fnv1a64(&k.side, sizeof(k.side));
  return x;
}

static inline double rate_with_bursts(int base_rate, double t, const std::vector<Burst>& bursts) {
  double r = base_rate;
  for (auto& b : bursts) { if (t >= b.t_s && t < (b.t_s + b.dur_s)) r *= b.x; }
  return r;
}

static EngineResult run_engine(const Args& args, bool deterministic_timing=false) {
  namespace fs = std::filesystem;
  fs::create_directories(args.report);

  Metrics m; m.seed=args.seed; m.code_hash=code_hash(); m.symbols=args.symbols; m.rate=args.rate; m.mode=args.mode; m.rss_mb = deterministic_timing ? 0.0 : rss_mb();
  LatencyRecorder& lat = m.latency;

  if (args.affinity) pin_to_cpu(*args.affinity);

  const int S = args.symbols;
  MdFeed feed(S, args.rate, args.seed, args.bursts, deterministic_timing);
  Strategy strat(S);
  Risk risk(S);

  // Trades CSV path
  std::string trades_csv = (fs::path(args.report)/"trades.csv").string();
  Router router(args.seed, trades_csv);

  // Queues
  struct Payload { MdEvent ev; };
  std::atomic<bool> done{false};
  std::atomic<uint64_t> drops{0};
  std::atomic<uint64_t> processed{0};
  std::atomic<uint64_t> seq{0};
  std::atomic<uint64_t> depth_max{0};

  auto start_tp = steady_clock::now();
  auto end_tp = start_tp + seconds(args.duration_s);

  // Naive queue
  std::queue<Payload> naive_q;
  std::mutex naive_m;

  // Optimized ring
  SpscRing<Payload> ring(1u<<14);

  auto producer = [&](){
    auto now = start_tp;
    double t = 0.0;
    while (now < end_tp) {
      double r = rate_with_bursts(args.rate, t, args.bursts);
      double period_ns = 1e9 / std::max(1.0, r);
      // produce one event per loop iteration
      MdEvent ev = feed.next(t);
      ev.ts_ns = to_ns(now);
      Payload p{ev};
      bool pushed=false;
      if (args.mode == "naive") {
        std::lock_guard<std::mutex> lk(naive_m);
        naive_q.push(p);
        pushed = true; // naive is unbounded (intentional), no drop here
      } else {
        pushed = ring.push(p);
        if (!pushed) drops++; else depth_max.store(std::max(depth_max.load(), (uint64_t)ring.depth()));
      }
      // Next schedule
      if (deterministic_timing) {
        now += nanoseconds((uint64_t)period_ns);
        t += period_ns/1e9;
      } else {
        // Sleep/yield until the next time; avoid busy spin
        now += nanoseconds((uint64_t)period_ns);
        t += period_ns/1e9;
        auto sleep_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now - steady_clock::now());
        if (sleep_ns.count() > 0) std::this_thread::sleep_for(sleep_ns);
      }
    }
    done.store(true);
  };

  auto consumer = [&](){
    OrderKey key{(uint64_t)args.seed, 0, 0, 0};
    while (!done.load() || (args.mode=="naive" ? !naive_q.empty() : ring.depth()>0)) {
      Payload p{};
      bool got=false;
      if (args.mode == "naive") {
        std::lock_guard<std::mutex> lk(naive_m);
        if (!naive_q.empty()) { p = naive_q.front(); naive_q.pop(); got=true; }
      } else {
        got = ring.pop(p);
      }
      if (!got) {
        if (!deterministic_timing) std::this_thread::yield();
        continue;
      }

      auto t0_ns = p.ev.ts_ns;
      // Strategy decision
      // Naive mode intentionally allocates in hot path to create tails
      Decision d = strat.on_mid(p.ev.symbol, p.ev.mid);
      if (args.mode == "naive") {
        // allocation and string manipulation as an intentional penalty
        std::string tmp = std::to_string(d.reason_score);
        if (tmp.size() > 1000000) std::cerr << "never"; // keep compiler from optimizing away
      }

      if (d.side != 0) {
        // Risk check
        auto riskr = risk.check(p.ev.symbol, d.side, d.qty, p.ev.mid);
        if (riskr.allowed) {
          key.sym = p.ev.symbol; key.seq = ++seq; key.side = d.side;
          uint64_t oid = make_order_id(key);
          router.ioc_fill(oid, p.ev.ts_ns, p.ev.symbol, d.side, d.qty, p.ev.mid, p.ev.spread*0.5, std::to_string(d.reason_score).substr(0,6));
          risk.on_fill(p.ev.symbol, d.side, d.qty, p.ev.mid);
        } else {
          // blocked
        }
      }
      auto t1 = deterministic_timing ? (t0_ns + 1000) : to_ns(steady_clock::now());
      double ms = ns_to_ms(t1 - t0_ns);
      lat.add_sample(ms);
      processed++;
      m.reliability.queue_depth_max = std::max<uint64_t>(m.reliability.queue_depth_max, depth_max.load());
    }
  };

  if (deterministic_timing) {
    // Single-threaded deterministic simulation
    producer();
    consumer();
  } else {
    std::thread pt(producer);
    std::thread ct(consumer);
    pt.join();
    ct.join();
  }

  // Throughput: processed / elapsed
  double elapsed_s = args.duration_s; // close enough; in real-time mode this will be ~duration
  m.eps = processed / std::max(1.0, elapsed_s);
  m.reliability.drops = drops.load();
  m.reliability.idempotency_violations = router.idempotency_violations();
  m.reliability.exposure_blocks = risk.exposure_blocks();
  if (!deterministic_timing) m.rss_mb = rss_mb(); else m.rss_mb = 0.0;

  // Artifacts
  std::ofstream f_json((std::filesystem::path(args.report)/"metrics.json").string());
  std::string json = m.to_json();
  f_json << json << std::endl;
  std::ofstream f_lat((std::filesystem::path(args.report)/"latency.csv").string());
  f_lat << lat.csv_samples_header() << "\n" << lat.csv_samples();
  std::ofstream f_fp((std::filesystem::path(args.report)/"run_fingerprint.txt").string());
  f_fp << "seed=" << args.seed << "\ncode_hash=" << code_hash() << "\nsymbols=" << args.symbols << "\nrate=" << args.rate << "\nmode=" << args.mode << "\n";
  std::ofstream f_md((std::filesystem::path(args.report)/"report.md").string());
  f_md << "Run report\n\n" << json << "\n";

  EngineResult er{m, json};
  return er;
}

static int determinism_check(const Args& args) {
  namespace fs = std::filesystem;
  fs::create_directories(args.report);
  std::vector<uint64_t> sums;
  for (int i=0;i<3;++i) {
    std::string run_dir = (fs::path(args.report)/("run"+std::to_string(i))).string();
    Args a = args; a.report = run_dir;
    auto er = run_engine(a, /*deterministic_timing=*/true);
    uint64_t sum = fnv1a64_str(er.metrics_json);
    sums.push_back(sum);
  }
  bool pass = (sums[0]==sums[1] && sums[1]==sums[2]);
  std::ofstream f((fs::path(args.report)/"determinism_result.json").string());
  f << "{ \"pass\": " << (pass?"true":"false") << ", \"runs\": [" << sums[0] << ", " << sums[1] << ", " << sums[2] << "] }\n";
  return pass ? 0 : 1;
}

} // namespace nhft

int main(int argc, char** argv) {
  using namespace nhft;
  auto args = parse_args(argc, argv);
  std::filesystem::create_directories(args.report);
  if (args.determinism_check) {
    return determinism_check(args);
  }
  auto er = run_engine(args, /*deterministic_timing=*/false);
  (void)er;
  return 0;
}

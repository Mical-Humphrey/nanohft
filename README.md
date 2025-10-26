# NanoHFT

Small, production-quality C++20 demo of HFT fundamentals you can run on a laptop in minutes. It contrasts a naive, allocation-heavy hot loop with an optimized SPSC ring-buffer design and reports tail latency, jitter, backpressure, safety gates, idempotency, and determinism.

## What it demonstrates

- Tail latency and jitter: p50/p95/p99/max, jitter_ratio = p99/p50
- Backpressure: bounded SPSC queue, drops, queue_depth_max
- Allocation/cache effects: naive vs optimized hot loop with measurable p95/p99 improvement
- Safety gates: per-trade notional cap and portfolio daily loss cap → exposure_blocks with reason
- Idempotent order flow: zero duplicate order IDs
- Determinism: fixed seed + code hash; 3-run determinism check with identical metrics checksum

## Build

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Quick demo (20s each)

```
./build/nanohft --duration-s 20 --mode naive --burst "t=10,dur=2,x=5" --report out/naive
./build/nanohft --duration-s 20 --mode optimized --burst "t=10,dur=2,x=5" --report out/opt
```

Open `out/naive/metrics.json` and `out/opt/metrics.json` and compare:

- latency_ms.p95
- latency_ms.p99
- latency_ms.jitter_ratio

Optimized should show lower tails (numbers vary by system). You can also run `scripts/run_demo.sh` to build and print key stats.

## Determinism check

```
./build/nanohft --determinism-check --duration-s 3 --report out/det
```

This produces `out/det/determinism_result.json` like:

```
{ "pass": true, "runs": [123456789, 123456789, 123456789] }
```

## CLI flags (defaults)

- `--duration-s INT` run duration in seconds (default 20)
- `--rate INT` aggregate events/sec across all symbols (default 100000)
- `--symbols INT` number of symbols (default 4)
- `--burst "t=10,dur=2,x=5"` boost rate by x in [t, t+dur) (repeatable)
- `--mode naive|optimized` queue and hot loop mode (default optimized)
- `--seed INT` RNG seed (default 7)
- `--affinity INT` best-effort CPU pin (Linux only)
- `--report PATH` output directory for artifacts (default ./out/run)
- `--determinism-check` run engine 3x with same params, write determinism_result.json

## Artifacts

- `metrics.json` latency percentiles, throughput, reliability counters, resources
- `latency.csv` up to 2000 latency samples (ms)
- `trades.csv` simulated IOC fills (if any)
- `run_fingerprint.txt` seed, code_hash, and params
- `report.md` brief run summary

## Interview talking points

- Why p99 matters; jitter vs mean
- Bounded vs unbounded queues; drop policy and backpressure
- Allocation and cacheline alignment impact on tails
- Safety gates and idempotency in trading workflows
- Determinism: seed + code hash + identical metrics checksums

## Notes

- Portable timing uses `std::chrono::steady_clock` in normal runs. Determinism mode uses synthetic timing to guarantee identical JSON across repeated runs with the same seed and params.
- Tests use a minimal embedded Catch2-compatible header to keep the project single-header and offline.

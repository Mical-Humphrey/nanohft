#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.."; pwd)
BUILD_DIR="$ROOT/build"

echo "[1/3] Build (Release)"
cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build "$BUILD_DIR" -j >/dev/null

echo "[2/3] Run naive vs optimized"
"$BUILD_DIR/nanohft" --duration-s 20 --mode naive --burst "t=10,dur=2,x=5" --report "$ROOT/out/naive"
"$BUILD_DIR/nanohft" --duration-s 20 --mode optimized --burst "t=10,dur=2,x=5" --report "$ROOT/out/opt"

extract_metrics() {
  f="$1/metrics.json"
  p95=$(grep -o '"p95": [0-9.]*' "$f" | awk '{print $2}')
  p99=$(grep -o '"p99": [0-9.]*' "$f" | awk '{print $2}')
  jr=$(grep -o '"jitter_ratio": [0-9.]*' "$f" | awk '{print $2}')
  echo "$p95 $p99 $jr"
}

read n_p95 n_p99 n_jr < <(extract_metrics "$ROOT/out/naive")
read o_p95 o_p99 o_jr < <(extract_metrics "$ROOT/out/opt")

echo "[3/3] Results"
printf "%-12s p95=%-8s p99=%-8s jitter=%-8s\n" "naive" "$n_p95" "$n_p99" "$n_jr"
printf "%-12s p95=%-8s p99=%-8s jitter=%-8s\n" "optimized" "$o_p95" "$o_p99" "$o_jr"

awk -v n="$n_p99" -v o="$o_p99" 'BEGIN { if (n>0) printf "p99 improvement: %.2fx faster\n", n/(o>0?o:1) }'

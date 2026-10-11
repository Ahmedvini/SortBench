import subprocess
import statistics
import random
import time
import csv
import os

sources = [
    "src/sort_fast_baseline.cpp",
    "src/sort_fast.cpp",
    "src/sort_fast_mt.cpp",
    "src/sort_fast_submit.cpp",
    "src/sort_fast_submit_mt.cpp",
]

rounds = 20
expected = "data/expected.txt"
input_file = "data/input.txt"
os.makedirs("bench_bins", exist_ok=True)

configs = []

for src in sources:
    for mode in (0, 1, 2):
        flags = ["-O3", "-march=native", "-std=c++17",
                 f"-DSORT_MODE={mode}"]

        if src == "src/sort_fast_mt.cpp":
            flags += ["-pthread", "-DMT_SER",
                      "-DMT_MAX_THREADS=8", "-DMT_FORCE=8"]
        elif src == "src/sort_fast_submit_mt.cpp":
            flags += ["-pthread", "-DMT_MAX_THREADS=8"]

        name = f"{src.removesuffix('.cpp')}_mode{mode}"
        binary = os.path.abspath(f"bench_bins/{name}")

        cmd = ["g++", *flags, src, "-o", binary]
        print(f"BUILD {name}", flush=True)
        build = subprocess.run(cmd, capture_output=True, text=True)

        if build.returncode != 0:
            print(f"BUILD FAILED: {name}\n{build.stderr}")
            continue

        configs.append((name, binary))

if not configs:
    raise SystemExit("No configuration built successfully.")

def run_and_check(binary, output):
    start = time.perf_counter_ns()
    p = subprocess.run(
        [binary, input_file, output],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE
    )
    elapsed = (time.perf_counter_ns() - start) / 1e6

    if p.returncode != 0:
        raise RuntimeError(
            f"Program failed: {binary}\n{p.stderr.decode(errors='replace')}"
        )

    check = subprocess.run(
        ["cmp", "-s", output, expected],
        check=False
    )
    if check.returncode != 0:
        raise RuntimeError(f"OUTPUT MISMATCH: {binary}")

    return elapsed

valid = []
for name, binary in configs:
    try:
        run_and_check(binary, "bench_warmup.txt")
        valid.append((name, binary))
        print(f"PASS {name}", flush=True)
    except Exception as e:
        print(f"INVALID {name}: {e}", flush=True)

if not valid:
    raise SystemExit("No configuration passed correctness checks.")

results = {name: [] for name, _ in valid}
tasks = [(name, binary) for _ in range(rounds)
         for name, binary in valid]
random.Random(20261011).shuffle(tasks)

for i, (name, binary) in enumerate(tasks, 1):
    try:
        ms = run_and_check(binary, "bench_output.txt")
        results[name].append(ms)
    except Exception as e:
        raise SystemExit(f"Benchmark failed at trial {i}: {e}")

    if i % 20 == 0:
        print(f"Progress: {i}/{len(tasks)} runs", flush=True)

summary = []
for name, _ in valid:
    values = results[name]
    summary.append({
        "configuration": name,
        "successful_runs": len(values),
        "median_ms": round(statistics.median(values), 3),
        "mean_ms": round(statistics.mean(values), 3),
        "min_ms": round(min(values), 3),
        "max_ms": round(max(values), 3),
    })

summary.sort(key=lambda row: row["median_ms"])

with open("benchmark_all_results.csv", "w", newline="") as f:
    writer = csv.DictWriter(f, fieldnames=summary[0].keys())
    writer.writeheader()
    writer.writerows(summary)

print("\n===== RANKING BY MEDIAN =====")
for rank, row in enumerate(summary, 1):
    print(
        f"{rank:2}. {row['configuration']:<38} "
        f"median={row['median_ms']:7.3f} ms  "
        f"mean={row['mean_ms']:7.3f} ms  "
        f"min={row['min_ms']:7.3f} ms  "
        f"max={row['max_ms']:7.3f} ms"
    )

print(f"\nAll measured outputs matched {expected}.")
print("CSV saved to benchmark_all_results.csv")

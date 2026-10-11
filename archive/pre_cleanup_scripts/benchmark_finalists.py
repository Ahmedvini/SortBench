import subprocess, statistics, random, time, csv, os

names = [
    "sort_fast_mt_mode1",
    "sort_fast_submit_mt_mode1",
    "sort_fast_mt_mode0",
    "sort_fast_submit_mt_mode0",
]

rounds = 40
os.makedirs("bench_bins", exist_ok=True)
bins = {n: os.path.abspath("bench_bins/" + n) for n in names}

for n, binary in bins.items():
    if not os.path.isfile(binary):
        raise SystemExit(f"Missing binary: {binary}")

def run(binary, output):
    start = time.perf_counter_ns()
    p = subprocess.run(
        [binary, "data/input.txt", output],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE
    )
    ms = (time.perf_counter_ns() - start) / 1e6

    if p.returncode:
        raise RuntimeError(p.stderr.decode(errors="replace"))

    if subprocess.run(["cmp", "-s", output, "data/expected.txt"]).returncode:
        raise RuntimeError(f"Incorrect output: {binary}")

    return ms

# Warm-up and correctness check
for n, binary in bins.items():
    run(binary, "bench_warmup.txt")
    print(f"PASS {n}", flush=True)

tasks = [(n, i) for i in range(1, rounds + 1) for n in names]
random.Random(20261012).shuffle(tasks)

raw = []
for i, (n, repetition) in enumerate(tasks, 1):
    ms = run(bins[n], "bench_output.txt")
    raw.append({"configuration": n, "repetition": repetition, "ms": ms})
    if i % 20 == 0:
        print(f"{i}/{len(tasks)}", flush=True)

with open("benchmark_finalists_raw.csv", "w", newline="") as f:
    w = csv.DictWriter(f, fieldnames=["configuration", "repetition", "ms"])
    w.writeheader()
    w.writerows(raw)

print("\nFINAL RANKING")
for n in sorted(names, key=lambda x: statistics.median(
        r["ms"] for r in raw if r["configuration"] == x)):
    vals = [r["ms"] for r in raw if r["configuration"] == n]
    print(
        f"{n:32} median={statistics.median(vals):.3f} ms  "
        f"mean={statistics.mean(vals):.3f} ms  "
        f"min={min(vals):.3f} ms  max={max(vals):.3f} ms"
    )

print("\nRaw measurements saved to benchmark_finalists_raw.csv")

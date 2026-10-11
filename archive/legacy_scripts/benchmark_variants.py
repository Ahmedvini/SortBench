import random
import statistics
import subprocess
import time

programs = {
    "Current": "./radix8_current",
    "No_SER4": "./radix8_no_ser4",
    "No_MMAP": "./radix8_no_mmap",
    "Plain": "./radix8_plain",
}

# Warm-up
for name, exe in programs.items():
    subprocess.run(
        [exe, "input.txt", f"warmup_{name}.txt"],
        check=True,
        stdout=subprocess.DEVNULL
    )

results = {name: [] for name in programs}
rng = random.Random(2026)

for round_no in range(30):
    order = list(programs.items())
    rng.shuffle(order)

    for name, exe in order:
        start = time.perf_counter_ns()
        subprocess.run(
            [exe, "input.txt", f"bench_{name}.txt"],
            check=True,
            stdout=subprocess.DEVNULL
        )
        elapsed_ms = (time.perf_counter_ns() - start) / 1e6
        results[name].append(elapsed_ms)

print("\nResults (30 runs each):")
for name, times in results.items():
    print(
        f"{name:10} "
        f"median={statistics.median(times):.3f} ms  "
        f"min={min(times):.3f} ms  "
        f"max={max(times):.3f} ms"
    )

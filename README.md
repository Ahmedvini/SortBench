# Large-Scale Evaluation of Modern Search and Sorting Algorithms

**A reproducible C++ benchmarking study of sorting implementations, performance variability, correctness, and parallel scalability.**

## Headline Results

| Metric | Result | Experimental context |
|---|---:|---|
| **Fastest recorded median** | **37.246 ms** | Comprehensive benchmark — `sort_fast_mt_mode1` |
| **Lowest recorded individual runtime** | **30.301 ms** | Separate finalist benchmark — `sort_fast_submit_mt_mode0` |
| Parallel speedup | **Up to 3.569×** | Separate parallel-scaling study; duplicate-heavy input, 8 threads |
| Comprehensive evaluation | **15 configurations, 300 measured trials** | 5 source implementations × 3 modes × 20 measured trials |
| Correctness validation | **All recorded checks passed** | Comprehensive benchmark's recorded execution |

> **Interpretation:** The 37.246 ms median and 30.301 ms minimum come from different configurations and experimental protocols. They must not be interpreted as the median and minimum of the same run or configuration. The parallel speedup results come from a separate scaling experiment.

---

## Overview

This project evaluates modern sorting implementations through controlled benchmarking, correctness checks, and performance analysis. It focuses on more than a single best-case runtime: the study considers median performance, run-to-run variability, input distributions, and the benefits and limitations of parallel execution.

The benchmarking engine is implemented in **C++**, with Python scripts for orchestration and result collection. The project includes compiled benchmark variants, deterministic input and reference-output files, and CSV summaries for subsequent analysis.

### Research Objectives

- Compare runtime performance across implementation variants and operating modes.
- Measure central tendency and variability rather than relying on a single timing.
- Validate sorted outputs against a reference result.
- Investigate parallel speedup and thread-scaling efficiency.
- Preserve benchmark results in machine-readable CSV files for analysis and reporting.

## Key Findings

### 1. Comprehensive Benchmark: Fastest Median

The comprehensive benchmark evaluated 15 configurations using 20 measured trials per configuration, giving **300 measured trials**.

The leading recorded median runtimes were:

| Rank | Configuration | Median runtime |
|---:|---|---:|
| 1 | `sort_fast_mt_mode1` | **37.246 ms** |
| 2 | `sort_fast_submit_mt_mode1` | 37.462 ms |
| 3 | `sort_fast_mt_mode0` | 37.627 ms |
| 4 | `sort_fast_submit_mt_mode0` | 39.479 ms |

The difference between the first and second configurations is **0.216 ms**, approximately **0.58%** of the second configuration's median. This is a small observed difference; without a dedicated statistical analysis, it should not be treated as evidence of a statistically significant performance advantage.

### 2. Minimum Runtime: Separate Finalist Benchmark

A separate finalist benchmark measured four configurations over 40 measured repetitions each.

| Configuration | Minimum runtime | Median runtime |
|---|---:|---:|
| `sort_fast_submit_mt_mode0` | **30.301 ms** | 39.921 ms |
| `sort_fast_mt_mode1` | 31.300 ms | 38.549 ms |
| `sort_fast_submit_mt_mode1` | 31.854 ms | 38.132 ms |
| `sort_fast_mt_mode0` | 31.219 ms | 40.565 ms |

The lowest observed runtime was **30.301 ms**, achieved by `sort_fast_submit_mt_mode0` in this finalist study. A minimum represents the fastest individual observation, not typical performance. The median is generally more informative for comparing typical runtime under repeated measurements.

**Important:** The finalist benchmark has a different repetition protocol from the comprehensive benchmark. Its medians therefore remain separate from the headline comprehensive results.

### 3. Parallel Scaling

A separate parallel-scaling study evaluated an eight-thread parallel merge-sort implementation against sequential Merge Sort on one million elements.

| Input distribution | Speedup at 8 threads | Parallel efficiency |
|---|---:|---:|
| Uniform | 3.046× | 38.1% |
| Sorted | 2.502× | 31.3% |
| Reverse sorted | 2.603× | 32.5% |
| Duplicate-heavy | **3.569×** | **44.6%** |
| Nearly sorted | 3.079× | 38.5% |

The highest recorded speedup was **3.569×** on duplicate-heavy input. The result demonstrates that the benefit of parallelization depends on input characteristics and that eight threads do not yield an eightfold speedup.

Parallel efficiency is calculated as:

\[
E_p = \frac{S_p}{p}\times 100\%
\]

where \(S_p\) is the measured speedup and \(p\) is the number of threads.

These results belong to the parallel-scaling study; they are not measurements from the 15-configuration comprehensive benchmark.

---

## Experimental Environment

The recorded benchmarking environment was:

| Component | Specification |
|---|---|
| Operating system | Pop!_OS 24.04 LTS |
| Kernel | Linux 6.18.7-76061807-generic |
| CPU | Intel Core i5-11400H |
| Logical CPUs | 12 |
| C++ compiler | GNU C++ 13.3.0 |
| Build system | CMake 3.28.3 |
| Optimization | `-O3 -DNDEBUG` in the recorded Release build |

The comprehensive benchmark's source compilation flags also include `-O3 -march=native -std=c++17` and a configuration-specific `SORT_MODE` definition. Parallel variants use thread-related compiler definitions and/or `-pthread` as specified by the benchmark script.

Results are hardware- and environment-dependent. Comparisons with other machines should account for differences in CPU architecture, compiler, compiler flags, operating-system scheduling, and system load.

## Dataset and Correctness

The project includes a dataset containing **1,000,000 randomly generated signed 32-bit integer values**, along with a sorted reference output.

Relevant files:

- `data/input.txt` — primary input dataset.
- `data/expected.txt` — sorted reference output.
- `data/edge.txt` — edge-case input.
- `data/edge_expected.txt` — expected output for the edge-case input.

The benchmark workflow validates produced outputs against the reference using `cmp`. The comprehensive benchmark's recorded execution passed its correctness checks.

Correctness validation is distinct from performance validation: matching the reference establishes output agreement for the tested input, but does not by itself establish general correctness for all possible inputs.

## Repository Structure

```text
sort-test/
├── benchmark_all.py
├── benchmark_finalists.py
├── bench_bins/
│   └── compiled benchmark executables
├── data/
│   ├── input.txt
│   ├── expected.txt
│   ├── edge.txt
│   └── edge_expected.txt
├── src/
│   ├── sort_fast_baseline.cpp
│   ├── sort_fast.cpp
│   ├── sort_fast_mt.cpp
│   ├── sort_fast_submit.cpp
│   └── sort_fast_submit_mt.cpp
├── results/
│   ├── generated_outputs/
│   └── summaries/
│       ├── benchmark_all_results.csv
│       └── benchmark_finalists_raw.csv
└── archive/
    ├── legacy_scripts/
    ├── old_executables/
    └── pre_cleanup_scripts/
```

`archive/` contains historical scripts, executables, and pre-cleanup script backups. `bench_bins/` contains precompiled executables used by the existing benchmark workflow.

## Running the Benchmarks

Run commands from the project root.

### Comprehensive benchmark

```bash
python3 benchmark_all.py
```

The script is configured for 15 configurations and 20 measured trials per configuration. Its intended output summary is:

```text
results/summaries/benchmark_all_results.csv
```

### Finalist benchmark

```bash
python3 benchmark_finalists.py
```

This script uses four existing compiled finalists and is configured for 40 measured repetitions per configuration. Its intended raw-results file is:

```text
results/summaries/benchmark_finalists_raw.csv
```

### Validate Python syntax

```bash
python3 -m py_compile benchmark_all.py benchmark_finalists.py
```

This checks Python syntax and compilation. It does **not** execute the full benchmark workflows.

**Reproducibility note:** The recorded results in this README reflect the available historical benchmark outputs. The full benchmark scripts have not been established as successfully rerun after the repository cleanup. Re-run each script, inspect its exit status and generated CSV, and verify correctness before describing a post-cleanup run as reproduced.

## Result Interpretation and Limitations

- **Median runtime** is the primary measure for typical performance in the comprehensive benchmark.
- **Minimum runtime** records the fastest observed trial and should not be treated as a stable performance guarantee.
- **Parallel speedup** is meaningful only with its baseline, input distribution, and thread count specified.
- **Correctness checks** cover the tested inputs and executions, not every possible input.
- Small differences in median runtime require further statistical analysis before claims of significance can be made.
- Reproducibility requires preserving the input data, source revisions, compiler flags, machine details, trial protocol, and raw measurements.
- The comprehensive, finalist, and parallel-scaling studies use distinct protocols; their results should not be combined as if they were collected under one experiment.

## Conclusion

The comprehensive benchmark recorded a best median runtime of **37.246 ms** for `sort_fast_mt_mode1` across 15 configurations and 300 measured trials. A separate finalist study recorded a minimum individual runtime of **30.301 ms** for `sort_fast_submit_mt_mode0`. In a separate parallel-scaling study, the eight-thread parallel merge-sort implementation achieved a maximum measured speedup of **3.569×** on duplicate-heavy input.

Together, these results provide three complementary views of performance: typical runtime across configurations, the lowest observed individual runtime, and parallel scalability across input distributions. Keeping these protocols distinct is essential for accurate interpretation and reproducible reporting.

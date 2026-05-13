# PDC-FinalProject — Parallel Sudoku Solver

CS 5350 Parallel and Distributed Computing  
Performance study comparing five Sudoku solving implementations across four difficulty levels.

---

## Algorithms

| Name       | Algorithm                   | Description                                    |
|------------|-----------------------------|------------------------------------------------|
| Serial     | Backtracking                | Brute-force serial baseline                    |
| Serial-MRV | Backtracking + MRV          | Serial with Minimum Remaining Values heuristic |
| OpenMP     | Backtracking + MRV          | Parallel tree search via OpenMP task spawning  |
| DLX        | Dancing Links (Algorithm X) | Exact cover serial solver                      |
| DLX-OMP    | Dancing Links + OpenMP      | DLX with parallel candidate evaluation         |

All solvers implement the same `Solver` interface and are driven by `BenchmarkRunner`, which writes results to a
timestamped CSV.

---

## Prerequisites

**macOS (Homebrew):**

```bash
brew install libomp
pip3 install pandas matplotlib scipy seaborn
```

**Ubuntu / WSL:**

```bash
sudo apt update && sudo apt install -y g++ cmake python3 python3-pip
pip3 install pandas matplotlib scipy seaborn
```

> `seaborn` is optional — the efficiency heatmap is skipped if it is not installed.

---

## Build

This project uses CMake. Build from the repo root:

```bash
cmake -S . -B build
cmake --build build
```

The binary is placed at `build/PDC_FinalProject`.

### HPC / cluster build (manual `-fopenmp`)

On systems where `find_package(OpenMP)` fails (e.g., some SLURM nodes):

```bash
cmake -S . -B build -DHPC=ON
cmake --build build
```

### macOS note

The build automatically searches `/opt/homebrew/opt/libomp` and `/usr/local/opt/libomp` for libomp.  
If OpenMP is not found the binary still builds, but all parallel solvers run serially.

---

## Run benchmarks

The binary runs the full benchmark suite automatically:

```bash
./build/PDC_FinalProject
```

This benchmarks all five solver configurations against 100 puzzles per difficulty level (Easy, Medium, Hard, Extreme)
with 5 repetitions each, using thread counts of 4, 8, and 16 for the parallel solvers.

Output CSV is written to `results/results_YYYYMMDD_HHMMSS.csv`.

### Configuration (edit top of `src/Benchmark/BenchmarkRunner.cpp`)

| Constant               | Default                        | Meaning                                      |
|------------------------|--------------------------------|----------------------------------------------|
| `OPENMP_THREAD_COUNTS` | `{4, 8, 16}`                   | Thread counts for OpenMP and DLX-OMP solvers |
| `REPETITIONS`          | `5`                            | Timed repetitions per puzzle per config      |
| `puzzleCount`          | `100` (passed from `main.cpp`) | Puzzles loaded per difficulty level          |

---

## Analyze results

```bash
python3 src/Scripts/analyze.py results/results_<timestamp>.csv
```

Produces plots and a summary table in `results/plots/`:

| File                       | Description                                                  |
|----------------------------|--------------------------------------------------------------|
| `speedup_vs_threads.png`   | Aggregate speedup curves per algorithm family and difficulty |
| `runtime_vs_threads.png`   | Solve time vs thread count (log scale)                       |
| `runtime_distribution.png` | Violin plots of solve time distribution per config           |
| `efficiency_heatmap.png`   | Parallel efficiency heatmaps per family *(requires seaborn)* |
| `all_configs_runtime.png`  | Grouped bar chart of average runtime across all configs      |
| `cost_vs_threads.png`      | Parallel cost C(P) = P × T(P) vs thread count                |
| `summary_table.csv`        | Aggregated median times, speedups, efficiencies              |

Speedup is computed within each algorithm family:

- **Backtracking**: OpenMP vs Serial
- **Backtracking-MRV**: OpenMP vs Serial-MRV
- **DLX**: DLX-OMP vs DLX serial

---

## Project structure

```
.
├── src/
│   ├── main.cpp                          # Entry point — calls BenchmarkRunner::run()
│   ├── Benchmark/
│   │   ├── BenchmarkRunner.cpp/.h        # Orchestrates all timing runs, writes CSV
│   │   └── PuzzleProfiler/              # Per-puzzle statistics helper
│   ├── BoardGenerator/
│   │   ├── BoardGenerator.cpp/.h         # Loads puzzles from PuzzleBank by difficulty
│   │   └── PuzzleBank.h                  # Hard-coded puzzle bank (Easy/Medium/Hard/Extreme)
│   ├── BoardSolver/
│   │   ├── Solver.h                      # Abstract base class for all solvers
│   │   ├── SerialSolver/                 # Brute-force backtracking (serial)
│   │   ├── SerialMRVSolver/              # Backtracking with MRV heuristic (serial)
│   │   ├── OpenMPSolver/                 # Parallel backtracking + MRV via OpenMP tasks
│   │   └── DLXSolver/                    # Dancing Links exact cover (serial + OpenMP)
│   ├── CorrectnessChecker/               # Validates solved board against Sudoku rules
│   ├── SudokuBoard/
│   │   ├── Board/                        # 9×9 board representation
│   │   ├── Block/                        # 3×3 block representation
│   │   └── CommonConstants.h             # Grid dimensions and shared constants
│   └── Scripts/
│       └── analyze.py                    # CSV → plots + summary table
├── results/
│   ├── results_*.csv                     # Raw timing output (git-ignored)
│   └── plots/                            # Generated plots and summary_table.csv
└── CMakeLists.txt
```

---

## CSV format

Each row in the output CSV represents one timed solve:

```
Board_ID,Difficulty,Algorithm,Threads,Rep,Time_s,Correct
easy_001,Easy,Serial,1,1,0.000123456,1
easy_001,Easy,OpenMP,4,1,0.000089123,1
...
```

`Correct` is `1` if the solved board passes the correctness checker, `0` otherwise.  
Only rows with `Correct=1` are used in timing plots.

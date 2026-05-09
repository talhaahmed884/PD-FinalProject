#!/usr/bin/env python3
"""
Sudoku Benchmark Analyzer
Usage: python3 analyze.py results_*.csv
"""

import matplotlib.pyplot as plt
import numpy as np
import os
import pandas as pd
import sys
import warnings

warnings.filterwarnings("ignore")

try:
    import seaborn as sns

    HAS_SEABORN = True
except ImportError:
    HAS_SEABORN = False
    print("NOTE: seaborn not installed — efficiency heatmap will be skipped.")
    print("      Install with: pip install seaborn")

# ============================================================
# CONFIG
# ============================================================
PLOT_DIR = "results/plots"
os.makedirs(PLOT_DIR, exist_ok=True)

DIFFICULTIES = ["Easy", "Medium", "Hard", "Extreme"]
THREAD_COUNTS = [4, 8, 16]

DIFF_COLORS = {
    "Easy": "#2ca02c",
    "Medium": "#ff7f0e",
    "Hard": "#1f77b4",
    "Extreme": "#d62728",
}

CONFIG_COLORS = {
    "Serial": "#555555",
    "OpenMP-4": "#ff7f0e",
    "OpenMP-8": "#2ca02c",
    "OpenMP-16": "#1f77b4",
}


# ============================================================
# DATA LOADING & PREPARATION
# ============================================================
def config_label(row):
    if row["Algorithm"] == "Serial":
        return "Serial"
    return f"OpenMP-{int(row['Threads'])}"


def load_data(paths):
    df = pd.concat([pd.read_csv(p) for p in paths], ignore_index=True)
    df["Config"] = df.apply(config_label, axis=1)
    print(f"Loaded {len(df)} total rows from {len(paths)} file(s).")
    return df


def compute_speedup(df):
    """Per-puzzle speedup: Serial time / OpenMP time for the same Board_ID."""
    serial = (
        df[df["Algorithm"] == "Serial"][["Board_ID", "Difficulty", "Time_s"]]
        .rename(columns={"Time_s": "T_serial"})
    )
    omp = df[df["Algorithm"] == "OpenMP"].copy()
    merged = omp.merge(serial, on=["Board_ID", "Difficulty"])
    merged["Speedup"] = merged["T_serial"] / merged["Time_s"]
    merged["Efficiency"] = merged["Speedup"] / merged["Threads"]
    return merged


# ============================================================
# PLOT 1 — Speedup vs Threads
# ============================================================
def plot_speedup_vs_threads(speedup_df):
    print("\n[Plot 1] Speedup vs Threads")

    agg = (
        speedup_df.groupby(["Difficulty", "Threads"])["Speedup"]
        .mean()
        .reset_index()
    )

    fig, ax = plt.subplots(figsize=(8, 5))
    ax.plot(THREAD_COUNTS, THREAD_COUNTS, "k--", lw=1, label="Ideal (linear)", zorder=1)

    for diff in DIFFICULTIES:
        s = agg[agg["Difficulty"] == diff].sort_values("Threads")
        if s.empty:
            continue
        ax.plot(
            s["Threads"], s["Speedup"],
            marker="o", color=DIFF_COLORS[diff], label=diff,
        )

    ax.set_xlabel("OpenMP Threads")
    ax.set_ylabel("Speedup  (T_serial / T_openmp)")
    ax.set_title("Speedup vs Thread Count by Difficulty")
    ax.set_xticks(THREAD_COUNTS)
    ax.legend()
    ax.grid(True, alpha=0.3)
    ax.set_xlim(left=0)
    ax.set_ylim(bottom=0)

    path = os.path.join(PLOT_DIR, "speedup_vs_threads.png")
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved: {path}")


# ============================================================
# PLOT 2 — Average Runtime by Difficulty and Config (grouped bar)
# ============================================================
def plot_avg_runtime(df):
    print("\n[Plot 2] Average Runtime by Difficulty and Config")

    configs = ["Serial"] + [f"OpenMP-{t}" for t in THREAD_COUNTS]
    agg = df.groupby(["Difficulty", "Config"])["Time_s"].mean().reset_index()

    x = np.arange(len(DIFFICULTIES))
    width = 0.18

    fig, ax = plt.subplots(figsize=(11, 6))

    for i, cfg in enumerate(configs):
        vals = [
            agg[(agg["Difficulty"] == d) & (agg["Config"] == cfg)]["Time_s"].values
            for d in DIFFICULTIES
        ]
        vals = [v[0] if len(v) else 0.0 for v in vals]
        ax.bar(
            x + i * width, vals, width,
            label=cfg, color=CONFIG_COLORS.get(cfg, "gray"),
        )

    ax.set_xlabel("Difficulty")
    ax.set_ylabel("Average Solve Time (seconds, log scale)")
    ax.set_title("Average Solve Time by Difficulty and Algorithm Configuration")
    ax.set_xticks(x + width * 1.5)
    ax.set_xticklabels(DIFFICULTIES)
    ax.set_yscale("log")
    ax.legend()
    ax.grid(True, alpha=0.3, axis="y")

    path = os.path.join(PLOT_DIR, "avg_runtime_by_difficulty.png")
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved: {path}")


# ============================================================
# PLOT 3 — Runtime Distribution Box Plots (one subplot per difficulty)
# ============================================================
def plot_runtime_distribution(df):
    print("\n[Plot 3] Runtime Distribution Box Plots")

    configs = ["Serial"] + [f"OpenMP-{t}" for t in THREAD_COUNTS]
    fig, axes = plt.subplots(1, len(DIFFICULTIES), figsize=(16, 5), sharey=False)
    fig.suptitle("Solve Time Distribution by Difficulty and Configuration", fontsize=13)

    for ax, diff in zip(axes, DIFFICULTIES):
        subset = df[df["Difficulty"] == diff]
        data = [subset[subset["Config"] == cfg]["Time_s"].dropna().values for cfg in configs]

        bp = ax.boxplot(data, patch_artist=True, labels=configs)
        for patch, cfg in zip(bp["boxes"], configs):
            patch.set_facecolor(CONFIG_COLORS.get(cfg, "gray"))
            patch.set_alpha(0.75)

        ax.set_title(diff, fontsize=11)
        ax.set_ylabel("Time (seconds)" if diff == DIFFICULTIES[0] else "")
        ax.set_yscale("log")
        ax.tick_params(axis="x", rotation=30)
        ax.grid(True, alpha=0.3, axis="y")

    fig.tight_layout()
    path = os.path.join(PLOT_DIR, "runtime_distribution.png")
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved: {path}")


# ============================================================
# PLOT 4 — Parallel Efficiency Heatmap
# ============================================================
def plot_efficiency_heatmap(speedup_df):
    if not HAS_SEABORN:
        print("\n[Plot 4] Efficiency heatmap — SKIPPED (seaborn not installed)")
        return

    print("\n[Plot 4] Parallel Efficiency Heatmap")

    agg = (
        speedup_df.groupby(["Difficulty", "Threads"])["Efficiency"]
        .mean()
        .reset_index()
    )
    pivot = (
        agg.pivot(index="Difficulty", columns="Threads", values="Efficiency")
        .reindex(DIFFICULTIES)
    )

    fig, ax = plt.subplots(figsize=(7, 4))
    sns.heatmap(
        pivot, annot=True, fmt=".2f", cmap="YlGn",
        vmin=0, vmax=1, ax=ax, linewidths=0.5,
        cbar_kws={"label": "Efficiency = Speedup / Threads"},
    )
    ax.set_title("Parallel Efficiency by Difficulty and Thread Count\n(ideal = 1.0)")
    ax.set_xlabel("OpenMP Threads")
    ax.set_ylabel("Difficulty")

    path = os.path.join(PLOT_DIR, "efficiency_heatmap.png")
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved: {path}")


# ============================================================
# PLOT 5 — Serial vs Best OpenMP Time per Difficulty (bar)
# ============================================================
def plot_serial_vs_best_omp(df):
    print("\n[Plot 5] Serial vs Best OpenMP Time")

    serial_avg = (
        df[df["Algorithm"] == "Serial"]
        .groupby("Difficulty")["Time_s"].mean()
        .reindex(DIFFICULTIES)
    )

    # Best OpenMP = fastest config (most threads) per difficulty
    best_omp = (
        df[(df["Algorithm"] == "OpenMP") & (df["Threads"] == THREAD_COUNTS[-1])]
        .groupby("Difficulty")["Time_s"].mean()
        .reindex(DIFFICULTIES)
    )

    x = np.arange(len(DIFFICULTIES))
    width = 0.35

    fig, ax = plt.subplots(figsize=(9, 5))
    ax.bar(x - width / 2, serial_avg.values, width, label="Serial",
           color=CONFIG_COLORS["Serial"])
    ax.bar(x + width / 2, best_omp.values, width,
           label=f"OpenMP-{THREAD_COUNTS[-1]}",
           color=CONFIG_COLORS[f"OpenMP-{THREAD_COUNTS[-1]}"])

    ax.set_xlabel("Difficulty")
    ax.set_ylabel("Average Solve Time (seconds, log scale)")
    ax.set_title(f"Serial vs OpenMP-{THREAD_COUNTS[-1]} Average Solve Time")
    ax.set_xticks(x)
    ax.set_xticklabels(DIFFICULTIES)
    ax.set_yscale("log")
    ax.legend()
    ax.grid(True, alpha=0.3, axis="y")

    path = os.path.join(PLOT_DIR, "serial_vs_best_omp.png")
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved: {path}")


# ============================================================
# PLOT 6 — Correctness Rate
# ============================================================
def plot_correctness(df):
    print("\n[Plot 6] Correctness Rate")

    configs = ["Serial"] + [f"OpenMP-{t}" for t in THREAD_COUNTS]
    agg = df.groupby(["Difficulty", "Config"])["Correct"].mean().reset_index()
    agg["Correct"] *= 100  # to percentage

    x = np.arange(len(DIFFICULTIES))
    width = 0.18

    fig, ax = plt.subplots(figsize=(11, 5))

    for i, cfg in enumerate(configs):
        vals = [
            agg[(agg["Difficulty"] == d) & (agg["Config"] == cfg)]["Correct"].values
            for d in DIFFICULTIES
        ]
        vals = [v[0] if len(v) else 0.0 for v in vals]
        ax.bar(
            x + i * width, vals, width,
            label=cfg, color=CONFIG_COLORS.get(cfg, "gray"),
        )

    ax.set_xlabel("Difficulty")
    ax.set_ylabel("Correctness Rate (%)")
    ax.set_title("Solver Correctness Rate by Difficulty and Configuration")
    ax.set_xticks(x + width * 1.5)
    ax.set_xticklabels(DIFFICULTIES)
    ax.set_ylim(0, 110)
    ax.axhline(100, color="gray", linestyle="--", lw=0.8)
    ax.legend()
    ax.grid(True, alpha=0.3, axis="y")

    path = os.path.join(PLOT_DIR, "correctness_rate.png")
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved: {path}")


# ============================================================
# SUMMARY TABLE
# ============================================================
def save_summary_table(df, speedup_df):
    print("\n[Table] Summary")

    time_agg = (
        df.groupby(["Difficulty", "Config"])["Time_s"]
        .agg(Time_mean="mean", Time_std="std", Time_min="min", Time_max="max")
        .reset_index()
    )

    speedup_agg = (
        speedup_df.groupby(["Difficulty", "Config"])["Speedup"]
        .mean()
        .reset_index()
        .rename(columns={"Speedup": "Speedup_mean"})
    )

    serial_ref = time_agg[time_agg["Config"] == "Serial"][["Difficulty", "Config"]].copy()
    serial_ref["Speedup_mean"] = 1.0
    speedup_agg = pd.concat([speedup_agg, serial_ref], ignore_index=True)

    summary = time_agg.merge(speedup_agg, on=["Difficulty", "Config"], how="left")

    config_order = ["Serial"] + [f"OpenMP-{t}" for t in THREAD_COUNTS]
    summary["Config"] = pd.Categorical(summary["Config"], categories=config_order, ordered=True)
    summary["Difficulty"] = pd.Categorical(summary["Difficulty"], categories=DIFFICULTIES, ordered=True)
    summary = summary.sort_values(["Difficulty", "Config"])

    path = os.path.join(PLOT_DIR, "summary_table.csv")
    summary.to_csv(path, index=False, float_format="%.6f")
    print(f"  Saved: {path}")

    pd.set_option("display.max_rows", 200)
    pd.set_option("display.float_format", "{:.6f}".format)
    pd.set_option("display.width", 120)
    print(summary.to_string(index=False))


# ============================================================
# MAIN
# ============================================================
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 analyze.py results_*.csv")
        sys.exit(1)

    df = load_data(sys.argv[1:])
    df_correct = df[df["Correct"] == 1].copy()
    print(f"  {len(df_correct)} rows with Correct=1 used for timing plots.")

    speedup_df = compute_speedup(df_correct)

    plot_speedup_vs_threads(speedup_df)
    plot_avg_runtime(df_correct)
    plot_runtime_distribution(df_correct)
    plot_efficiency_heatmap(speedup_df)
    plot_serial_vs_best_omp(df_correct)
    plot_correctness(df)
    save_summary_table(df_correct, speedup_df)

    print(f"\nAll plots written to: {PLOT_DIR}/")

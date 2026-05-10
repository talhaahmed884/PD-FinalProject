#!/usr/bin/env python3
"""
Sudoku Benchmark Analyzer
Usage: python3 analyze.py results_*.csv
"""

import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
import numpy as np
import os
import pandas as pd
import sys
import warnings

warnings.filterwarnings("ignore", category=FutureWarning)

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
    raw = pd.concat([pd.read_csv(p) for p in paths], ignore_index=True)
    raw = raw[raw["Time_s"] > 0].copy()
    print(f"Loaded {len(raw)} raw timing rows from {len(paths)} file(s).")

    # Aggregate repetitions per puzzle × config — same pattern as the MM benchmark script.
    # Each rep is a separate row in the CSV; we reduce to median + variance stats here.
    group_keys = ["Board_ID", "Difficulty", "Algorithm", "Threads"]
    time_agg = (
        raw.groupby(group_keys)["Time_s"]
        .agg(Time_s="median", Time_min="min", Time_max="max", Time_std="std")
        .reset_index()
    )
    # Correct=1 if the solver produced the right answer on any rep
    correct_agg = raw.groupby(group_keys)["Correct"].max().reset_index()

    df = time_agg.merge(correct_agg, on=group_keys)
    df["Config"] = df.apply(config_label, axis=1)
    df["Cost"] = df["Threads"] * df["Time_s"]

    print(f"  Aggregated to {len(df)} config rows (median across reps).")
    return df


def compute_speedup(df):
    """
    Per-puzzle speedup and efficiency relative to the serial baseline.
    Used for Q1-Q3 variance bands and per-puzzle distribution analysis.

    Aggregate speedup S_agg = sum(T_serial) / sum(T_parallel) is computed
    separately inside each plot function to avoid mean-of-ratios inflation.
    """
    serial = (
        df[df["Algorithm"] == "Serial"][["Board_ID", "Difficulty", "Time_s"]]
        .rename(columns={"Time_s": "T_serial"})
    )
    omp = df[df["Algorithm"] == "OpenMP"].copy()
    merged = omp.merge(serial, on=["Board_ID", "Difficulty"])
    merged["Speedup"] = merged["T_serial"] / merged["Time_s"]
    merged["Efficiency"] = merged["Speedup"] / merged["Threads"]
    merged["Cost"] = merged["Threads"] * merged["Time_s"]
    return merged


def _aggregate_speedup(speedup_df):
    """
    Aggregate speedup: sum(T_serial) / sum(T_parallel) per (Difficulty, Threads).
    More honest than mean(T_serial/T_parallel), which is inflated by outlier puzzles
    where serial is very slow but parallel gets lucky.
    Uses an explicit loop to avoid pandas version issues with groupby key exclusion.
    """
    rows = []
    for (diff, threads), g in speedup_df.groupby(["Difficulty", "Threads"]):
        agg_speedup = g["T_serial"].sum() / g["Time_s"].sum()
        rows.append({
            "Difficulty": diff,
            "Threads": threads,
            "agg_speedup": agg_speedup,
            "agg_efficiency": agg_speedup / threads,
            "lo": g["Speedup"].quantile(0.25),
            "hi": g["Speedup"].quantile(0.75),
        })
    return pd.DataFrame(rows)


# ============================================================
# PLOT 1 — Speedup vs Threads
# Line = aggregate speedup S_agg = sum(T_serial) / sum(T_parallel).
# Shaded band = Q1-Q3 of per-puzzle speedup (shows puzzle-to-puzzle variance).
# ============================================================
def plot_speedup_vs_threads(speedup_df):
    print("\n[Plot 1] Speedup vs Threads")

    agg = _aggregate_speedup(speedup_df)

    fig, ax = plt.subplots(figsize=(8, 5))
    ax.plot(THREAD_COUNTS, THREAD_COUNTS, "k--", lw=1, label="Ideal (linear)", zorder=1)

    for diff in DIFFICULTIES:
        s = agg[agg["Difficulty"] == diff].sort_values("Threads")
        if s.empty:
            continue
        ax.plot(s["Threads"], s["agg_speedup"], marker="o",
                color=DIFF_COLORS[diff], label=diff, zorder=2)
        ax.fill_between(
            s["Threads"], s["lo"], s["hi"],
            color=DIFF_COLORS[diff], alpha=0.12,
        )

    ax.set_xlabel("OpenMP Threads")
    ax.set_ylabel("Speedup  S = Σ T_serial / Σ T_parallel")
    ax.set_title("Speedup vs Thread Count by Difficulty\n"
                 "(line = aggregate speedup; band = Q1–Q3 per-puzzle)")
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
# PLOT 2 — Runtime vs Thread Count
# ============================================================
def plot_runtime_vs_threads(df):
    print("\n[Plot 2] Runtime vs Thread Count")

    agg = df.groupby(["Difficulty", "Threads"])["Time_s"].mean().reset_index()
    all_threads = sorted(df["Threads"].unique())

    fig, ax = plt.subplots(figsize=(8, 5))

    for diff in DIFFICULTIES:
        s = agg[agg["Difficulty"] == diff].sort_values("Threads")
        if s.empty:
            continue
        ax.plot(s["Threads"], s["Time_s"], marker="o", color=DIFF_COLORS[diff], label=diff)

    ax.set_xlabel("Threads  (1 = Serial)")
    ax.set_ylabel("Average Solve Time (seconds)")
    ax.set_title("Runtime vs Thread Count by Difficulty")
    ax.set_xticks(all_threads)
    ax.set_yscale("log")
    ax.legend()
    ax.grid(True, alpha=0.3, which="both")

    path = os.path.join(PLOT_DIR, "runtime_vs_threads.png")
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved: {path}")


# ============================================================
# PLOT 3 — Runtime Distribution — Violin Plots
# Violin plots on log-transformed data show the full distribution shape
# (bimodality, skew) that box plots hide.  Each difficulty gets one
# subplot; each configuration gets one violin.
# ============================================================
def plot_runtime_distribution(df):
    print("\n[Plot 3] Runtime Distribution Violin Plots")

    configs = ["Serial"] + [f"OpenMP-{t}" for t in THREAD_COUNTS]
    fig, axes = plt.subplots(1, len(DIFFICULTIES), figsize=(16, 5), sharey=False)
    fig.suptitle("Solve Time Distribution by Difficulty and Configuration", fontsize=13)

    for ax, diff in zip(axes, DIFFICULTIES):
        subset = df[df["Difficulty"] == diff]

        # Log-transform so violin kernel density makes sense on a skewed distribution
        log_data = [
            np.log10(subset[subset["Config"] == cfg]["Time_s"].dropna().values)
            for cfg in configs
        ]

        parts = ax.violinplot(
            log_data,
            positions=range(len(configs)),
            showmedians=True,
            showextrema=True,
        )

        for body, cfg in zip(parts["bodies"], configs):
            body.set_facecolor(CONFIG_COLORS.get(cfg, "#888888"))
            body.set_alpha(0.72)
            body.set_edgecolor("black")
            body.set_linewidth(0.5)

        for key in ("cbars", "cmins", "cmaxes"):
            if key in parts:
                parts[key].set_color("black")
                parts[key].set_linewidth(0.8)
        if "cmedians" in parts:
            parts["cmedians"].set_color("white")
            parts["cmedians"].set_linewidth(2.0)

        ax.set_xticks(range(len(configs)))
        ax.set_xticklabels(configs, rotation=30, ha="right", fontsize=8)
        ax.set_title(diff, fontsize=11)
        ax.set_ylabel("Time (seconds)" if diff == DIFFICULTIES[0] else "")

        # Re-label y-axis: data is log10(seconds), show as 10^n labels
        lo, hi = ax.get_ylim()
        tick_pows = [p for p in range(-5, 2) if lo - 0.15 <= p <= hi + 0.15]
        if tick_pows:
            ax.set_yticks(tick_pows)
            ax.set_yticklabels([f"$10^{{{p}}}$" for p in tick_pows])

        ax.grid(True, alpha=0.3, axis="y")

    fig.tight_layout()
    path = os.path.join(PLOT_DIR, "runtime_distribution.png")
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved: {path}")


# ============================================================
# PLOT 4 — Parallel Efficiency Heatmap
# Uses aggregate efficiency E_agg = S_agg / P.
# vmax is set to the actual data maximum so the colormap is not
# clipped at 1.0, which would make superlinear cells indistinguishable.
# ============================================================
def plot_efficiency_heatmap(speedup_df):
    if not HAS_SEABORN:
        print("\n[Plot 4] Efficiency heatmap — SKIPPED (seaborn not installed)")
        return

    print("\n[Plot 4] Parallel Efficiency Heatmap")

    agg = _aggregate_speedup(speedup_df)
    pivot = (
        agg.pivot(index="Difficulty", columns="Threads", values="agg_efficiency")
        .reindex(DIFFICULTIES)
    )

    fig, ax = plt.subplots(figsize=(7, 4))
    sns.heatmap(
        pivot, annot=True, fmt=".2f", cmap="YlGn",
        vmin=0, vmax=1.0, ax=ax, linewidths=0.5,
        cbar_kws={"label": "Aggregate Efficiency  E = S_agg / P"},
    )
    ax.set_title("Parallel Efficiency by Difficulty and Thread Count\n"
                 "(aggregate: ideal = 1.0)")
    ax.set_xlabel("OpenMP Threads")
    ax.set_ylabel("Difficulty")

    path = os.path.join(PLOT_DIR, "efficiency_heatmap.png")
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved: {path}")


# ============================================================
# PLOT 5 — Serial vs All OpenMP Configurations
# Grouped bar chart: one group per difficulty, one bar per config.
# Shows the full picture across all thread counts, not just the highest.
# ============================================================
def plot_serial_vs_all_omp(df):
    print("\n[Plot 5] Serial vs All OpenMP Configurations")

    configs = ["Serial"] + [f"OpenMP-{t}" for t in THREAD_COUNTS]
    avg = df.groupby(["Difficulty", "Config"])["Time_s"].mean().reset_index()

    x = np.arange(len(DIFFICULTIES))
    n = len(configs)
    width = 0.72 / n

    fig, ax = plt.subplots(figsize=(11, 5))

    for i, cfg in enumerate(configs):
        vals = (
            avg[avg["Config"] == cfg]
            .set_index("Difficulty")
            .reindex(DIFFICULTIES)["Time_s"]
            .values
        )
        offset = (i - n / 2 + 0.5) * width
        ax.bar(
            x + offset, vals, width * 0.92,
            label=cfg,
            color=CONFIG_COLORS.get(cfg, "#888888"),
        )

    ax.set_xlabel("Difficulty")
    ax.set_ylabel("Average Solve Time (seconds, log scale)")
    ax.set_title("Average Solve Time: Serial vs All OpenMP Configurations")
    ax.set_xticks(x)
    ax.set_xticklabels(DIFFICULTIES)
    ax.set_yscale("log")
    ax.legend()
    ax.grid(True, alpha=0.3, axis="y")

    path = os.path.join(PLOT_DIR, "serial_vs_all_omp.png")
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved: {path}")


# ============================================================
# PLOT 6 — Cost vs Threads
# ============================================================
def plot_cost_vs_threads(df):
    print("\n[Plot 6] Cost vs Threads")

    agg = df.groupby(["Difficulty", "Threads"])["Cost"].mean().reset_index()
    all_threads = sorted(df["Threads"].unique())

    serial_cost = (
        df[df["Threads"] == 1]
        .groupby("Difficulty")["Cost"].mean()
    )

    fig, ax = plt.subplots(figsize=(8, 5))

    for diff in DIFFICULTIES:
        s = agg[agg["Difficulty"] == diff].sort_values("Threads")
        if s.empty:
            continue
        ax.plot(s["Threads"], s["Cost"], marker="s", color=DIFF_COLORS[diff], label=diff, zorder=2)
        if diff in serial_cost.index:
            ax.axhline(
                serial_cost[diff],
                color=DIFF_COLORS[diff], lw=1, linestyle="--", alpha=0.5, zorder=1,
            )

    dash_proxy = mpatches.Patch(
        facecolor="none", edgecolor="gray", linestyle="--", label="— serial C(1) reference"
    )
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles + [dash_proxy], labels=labels + ["— serial C(1) reference"])

    ax.set_xlabel("Threads  (1 = Serial)")
    ax.set_ylabel("Cost  C(P) = P × T(P)  (thread-seconds)")
    ax.set_title("Parallel Cost vs Thread Count by Difficulty\n"
                 "Data line ≈ dashed reference → cost-optimal  |  Rising line → growing overhead")
    ax.set_xticks(all_threads)
    ax.set_yscale("log")
    ax.grid(True, alpha=0.3, which="both")

    path = os.path.join(PLOT_DIR, "cost_vs_threads.png")
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved: {path}")


# ============================================================
# SUMMARY TABLE
# Reports both aggregate speedup/efficiency (S_agg = ΣT_serial/ΣT_parallel)
# and mean per-puzzle speedup alongside the timing statistics.
# ============================================================
def save_summary_table(df, speedup_df):
    print("\n[Table] Summary")

    time_agg = (
        df.groupby(["Difficulty", "Config"])["Time_s"]
        .agg(
            Time_median="median",
            Time_mean="mean",
            Time_std="std",
            Time_min="min",
            Time_max="max",
        )
        .reset_index()
    )

    cost_agg = (
        df.groupby(["Difficulty", "Config"])["Cost"]
        .mean()
        .reset_index()
        .rename(columns={"Cost": "Cost_mean"})
    )

    # Aggregate speedup/efficiency (honest metric)
    agg_rows = []
    for (diff, config), g in speedup_df.groupby(["Difficulty", "Config"]):
        threads = g["Threads"].iloc[0]
        agg_speedup = g["T_serial"].sum() / g["Time_s"].sum()
        agg_rows.append({
            "Difficulty": diff,
            "Config": config,
            "Agg_Speedup": agg_speedup,
            "Agg_Efficiency": agg_speedup / threads,
        })
    agg_metrics = pd.DataFrame(agg_rows)

    # Mean per-puzzle speedup/efficiency (kept for reference, but note it is inflated by outliers)
    mean_speedup = (
        speedup_df.groupby(["Difficulty", "Config"])["Speedup"]
        .mean()
        .reset_index()
        .rename(columns={"Speedup": "Speedup_mean"})
    )
    mean_efficiency = (
        speedup_df.groupby(["Difficulty", "Config"])["Efficiency"]
        .mean()
        .reset_index()
        .rename(columns={"Efficiency": "Efficiency_mean"})
    )

    # Serial reference rows: speedup = 1.0, efficiency = 1.0 by definition
    serial_ref = time_agg[time_agg["Config"] == "Serial"][["Difficulty", "Config"]].copy()
    serial_ref_agg = serial_ref.copy()
    serial_ref_agg["Agg_Speedup"] = 1.0
    serial_ref_agg["Agg_Efficiency"] = 1.0
    agg_metrics = pd.concat([agg_metrics, serial_ref_agg], ignore_index=True)

    serial_ref["Speedup_mean"] = 1.0
    mean_speedup = pd.concat([mean_speedup, serial_ref[["Difficulty", "Config", "Speedup_mean"]]], ignore_index=True)
    serial_ref["Efficiency_mean"] = 1.0
    mean_efficiency = pd.concat([mean_efficiency, serial_ref[["Difficulty", "Config", "Efficiency_mean"]]],
                                ignore_index=True)

    summary = (
        time_agg
        .merge(cost_agg, on=["Difficulty", "Config"], how="left")
        .merge(agg_metrics, on=["Difficulty", "Config"], how="left")
        .merge(mean_speedup, on=["Difficulty", "Config"], how="left")
        .merge(mean_efficiency, on=["Difficulty", "Config"], how="left")
    )

    config_order = ["Serial"] + [f"OpenMP-{t}" for t in THREAD_COUNTS]
    summary["Config"] = pd.Categorical(summary["Config"], categories=config_order, ordered=True)
    summary["Difficulty"] = pd.Categorical(summary["Difficulty"], categories=DIFFICULTIES, ordered=True)
    summary = summary.sort_values(["Difficulty", "Config"])

    path = os.path.join(PLOT_DIR, "summary_table.csv")
    summary.to_csv(path, index=False, float_format="%.6f")
    print(f"  Saved: {path}")

    pd.set_option("display.max_rows", 200)
    pd.set_option("display.float_format", "{:.4f}".format)
    pd.set_option("display.width", 200)
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
    speedup_df["Config"] = speedup_df.apply(config_label, axis=1)

    plot_speedup_vs_threads(speedup_df)
    plot_runtime_vs_threads(df_correct)
    plot_runtime_distribution(df_correct)
    plot_efficiency_heatmap(speedup_df)
    plot_serial_vs_all_omp(df_correct)
    plot_cost_vs_threads(df_correct)
    save_summary_table(df_correct, speedup_df)

    print(f"\nAll plots written to: {PLOT_DIR}/")

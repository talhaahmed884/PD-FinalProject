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
    "Serial-MRV": "#795548",
    "OpenMP-4": "#ff7f0e",
    "OpenMP-8": "#2ca02c",
    "OpenMP-16": "#1f77b4",
    "OMP-Frontier-4": "#bcbd22",
    "OMP-Frontier-8": "#f7b6d2",
    "OMP-Frontier-16": "#aec7e8",
    "DLX": "#9467bd",
    "DLX-OMP-4": "#e377c2",
    "DLX-OMP-8": "#8c564b",
    "DLX-OMP-16": "#17becf",
}


# ============================================================
# DATA LOADING & PREPARATION
# ============================================================
def config_label(row):
    alg = row["Algorithm"]
    if alg == "Serial":
        return "Serial"
    elif alg == "Serial-MRV":
        return "Serial-MRV"
    elif alg == "OpenMP":
        return f"OpenMP-{int(row['Threads'])}"
    elif alg == "OMP-Frontier":
        return f"OMP-Frontier-{int(row['Threads'])}"
    elif alg == "DLX":
        return "DLX"
    else:  # DLX-OMP
        return f"DLX-OMP-{int(row['Threads'])}"


def algorithm_family(alg):
    if alg in ("DLX", "DLX-OMP"):
        return "DLX"
    elif alg == "Serial-MRV":
        return "Backtracking-MRV"
    elif alg == "OMP-Frontier":
        return "Backtracking-Frontier"
    else:
        return "Backtracking"


def load_data(paths):
    raw = pd.concat([pd.read_csv(p) for p in paths], ignore_index=True)
    raw = raw[raw["Time_s"] > 0].copy()
    print(f"Loaded {len(raw)} raw timing rows from {len(paths)} file(s).")

    group_keys = ["Board_ID", "Difficulty", "Algorithm", "Threads"]
    time_agg = (
        raw.groupby(group_keys)["Time_s"]
        .agg(Time_s="median", Time_min="min", Time_max="max", Time_std="std")
        .reset_index()
    )
    correct_agg = raw.groupby(group_keys)["Correct"].max().reset_index()

    df = time_agg.merge(correct_agg, on=group_keys)
    df["Config"] = df.apply(config_label, axis=1)
    df["Family"] = df["Algorithm"].apply(algorithm_family)
    df["Cost"] = df["Threads"] * df["Time_s"]

    print(f"  Aggregated to {len(df)} config rows (median across reps).")
    return df


def compute_speedup(df):
    """
    Compute speedup separately for each algorithm family:
      Backtracking:     OpenMP-N vs Serial (brute-force)
      Backtracking-MRV: OpenMP-N vs Serial-MRV
      DLX:              DLX-OMP-N vs DLX serial
    Returns combined DataFrame with a Family column.
    """
    results = []

    # Backtracking family: OpenMP vs brute-force Serial
    serial = (
        df[df["Algorithm"] == "Serial"][["Board_ID", "Difficulty", "Time_s"]]
        .rename(columns={"Time_s": "T_serial"})
    )
    omp = df[df["Algorithm"] == "OpenMP"].copy()
    if not omp.empty and not serial.empty:
        merged = omp.merge(serial, on=["Board_ID", "Difficulty"])
        merged["Speedup"] = merged["T_serial"] / merged["Time_s"]
        merged["Efficiency"] = merged["Speedup"] / merged["Threads"]
        merged["Family"] = "Backtracking"
        results.append(merged)

    # Backtracking-MRV family: OpenMP vs Serial-MRV
    serial_mrv = (
        df[df["Algorithm"] == "Serial-MRV"][["Board_ID", "Difficulty", "Time_s"]]
        .rename(columns={"Time_s": "T_serial"})
    )
    omp_mrv = df[df["Algorithm"] == "OpenMP"].copy()
    if not omp_mrv.empty and not serial_mrv.empty:
        merged = omp_mrv.merge(serial_mrv, on=["Board_ID", "Difficulty"])
        merged["Speedup"] = merged["T_serial"] / merged["Time_s"]
        merged["Efficiency"] = merged["Speedup"] / merged["Threads"]
        merged["Family"] = "Backtracking-MRV"
        results.append(merged)

    # DLX family: DLX-OMP vs DLX serial
    dlx_serial = (
        df[df["Algorithm"] == "DLX"][["Board_ID", "Difficulty", "Time_s"]]
        .rename(columns={"Time_s": "T_serial"})
    )
    dlx_omp = df[df["Algorithm"] == "DLX-OMP"].copy()
    if not dlx_omp.empty and not dlx_serial.empty:
        merged = dlx_omp.merge(dlx_serial, on=["Board_ID", "Difficulty"])
        merged["Speedup"] = merged["T_serial"] / merged["Time_s"]
        merged["Efficiency"] = merged["Speedup"] / merged["Threads"]
        merged["Family"] = "DLX"
        results.append(merged)

    # Backtracking-Frontier family: OMP-Frontier vs Serial-MRV
    omp_frontier = df[df["Algorithm"] == "OMP-Frontier"].copy()
    if not omp_frontier.empty and not serial_mrv.empty:
        merged = omp_frontier.merge(serial_mrv, on=["Board_ID", "Difficulty"])
        merged["Speedup"] = merged["T_serial"] / merged["Time_s"]
        merged["Efficiency"] = merged["Speedup"] / merged["Threads"]
        merged["Family"] = "Backtracking-Frontier"
        results.append(merged)

    return pd.concat(results, ignore_index=True) if results else pd.DataFrame()


def _aggregate_speedup(speedup_df, family=None):
    """
    Aggregate speedup: sum(T_serial) / sum(T_parallel) per (Difficulty, Threads).
    Pass family= to filter by algorithm family.
    """
    subset = speedup_df if family is None else speedup_df[speedup_df["Family"] == family]
    rows = []
    for (diff, threads), g in subset.groupby(["Difficulty", "Threads"]):
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
# PLOT 1 — Speedup vs Threads (one subplot per family)
# ============================================================
def plot_speedup_vs_threads(speedup_df):
    print("\n[Plot 1] Speedup vs Threads")

    if speedup_df.empty:
        print("  Skipped — no speedup data.")
        return

    families = [f for f in ["Backtracking", "Backtracking-MRV", "Backtracking-Frontier", "DLX"]
                if f in speedup_df["Family"].values]
    fig, axes = plt.subplots(1, len(families), figsize=(7 * len(families), 5), sharey=False)
    if len(families) == 1:
        axes = [axes]

    subtitles = {
        "Backtracking": "Algorithm A — Backtracking (OpenMP vs Serial)",
        "Backtracking-MRV": "Algorithm A — Backtracking (OpenMP vs Serial-MRV)",
        "Backtracking-Frontier": "Algorithm A — Backtracking (OMP-Frontier vs Serial-MRV)",
        "DLX": "Algorithm B — DLX (DLX-OMP vs DLX Serial)",
    }

    for ax, family in zip(axes, families):
        agg = _aggregate_speedup(speedup_df, family=family)
        ax.plot(THREAD_COUNTS, THREAD_COUNTS, "k--", lw=1, label="Ideal (linear)", zorder=1)

        for diff in DIFFICULTIES:
            s = agg[agg["Difficulty"] == diff].sort_values("Threads")
            if s.empty:
                continue
            ax.plot(s["Threads"], s["agg_speedup"], marker="o",
                    color=DIFF_COLORS[diff], label=diff, zorder=2)
            ax.fill_between(s["Threads"], s["lo"], s["hi"],
                            color=DIFF_COLORS[diff], alpha=0.12)

        ax.set_xlabel("Threads")
        ax.set_ylabel("Speedup  S = Σ T_serial / Σ T_parallel")
        ax.set_title(subtitles[family])
        ax.set_xticks(THREAD_COUNTS)
        ax.legend()
        ax.grid(True, alpha=0.3)
        ax.set_xlim(left=0)
        ax.set_ylim(bottom=0)

    fig.suptitle("Speedup vs Thread Count by Difficulty\n"
                 "(line = aggregate speedup; band = Q1–Q3 per-puzzle)", fontsize=13)
    fig.tight_layout()
    path = os.path.join(PLOT_DIR, "speedup_vs_threads.png")
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved: {path}")


# ============================================================
# PLOT 2 — Runtime vs Threads (one subplot per family)
# ============================================================
def plot_runtime_vs_threads(df):
    print("\n[Plot 2] Runtime vs Threads")

    families = {
        "Backtracking": ("Serial", "OpenMP"),
        "Backtracking-MRV": ("Serial-MRV", "OpenMP"),
        "Backtracking-Frontier": ("Serial-MRV", "OMP-Frontier"),
        "DLX": ("DLX", "DLX-OMP"),
    }
    subtitles = {
        "Backtracking": "Algorithm A — Backtracking",
        "Backtracking-MRV": "Algorithm A — Backtracking (MRV baseline)",
        "Backtracking-Frontier": "Algorithm A — Backtracking (Frontier vs Serial-MRV)",
        "DLX": "Algorithm B — DLX",
    }

    fig, axes = plt.subplots(1, 4, figsize=(28, 5), sharey=False)

    for ax, (family, (s_alg, p_alg)) in zip(axes, families.items()):
        for diff in DIFFICULTIES:
            x_vals, y_vals = [], []

            s = df[(df["Algorithm"] == s_alg) & (df["Difficulty"] == diff)]
            if not s.empty:
                x_vals.append(1)
                y_vals.append(s["Time_s"].mean())

            for t in THREAD_COUNTS:
                p = df[(df["Algorithm"] == p_alg) & (df["Difficulty"] == diff) & (df["Threads"] == t)]
                if not p.empty:
                    x_vals.append(t)
                    y_vals.append(p["Time_s"].mean())

            if x_vals:
                ax.plot(x_vals, y_vals, marker="o", color=DIFF_COLORS[diff], label=diff)

        ax.set_xlabel("Threads  (1 = serial baseline)")
        ax.set_ylabel("Average Solve Time (seconds)")
        ax.set_title(subtitles[family])
        ax.set_xticks([1] + THREAD_COUNTS)
        ax.set_yscale("log")
        ax.legend()
        ax.grid(True, alpha=0.3, which="both")

    fig.suptitle("Runtime vs Thread Count by Difficulty", fontsize=13)
    fig.tight_layout()
    path = os.path.join(PLOT_DIR, "runtime_vs_threads.png")
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved: {path}")


# ============================================================
# PLOT 3 — Runtime Distribution — Violin Plots
# ============================================================
def plot_runtime_distribution(df):
    print("\n[Plot 3] Runtime Distribution Violin Plots")

    all_configs = (
            ["Serial", "Serial-MRV", "DLX"]
            + [f"OpenMP-{t}" for t in THREAD_COUNTS]
            + [f"OMP-Frontier-{t}" for t in THREAD_COUNTS]
            + [f"DLX-OMP-{t}" for t in THREAD_COUNTS]
    )
    configs = [c for c in all_configs if c in df["Config"].unique()]

    fig, axes = plt.subplots(1, len(DIFFICULTIES), figsize=(20, 5), sharey=False)
    fig.suptitle("Solve Time Distribution by Difficulty and Configuration", fontsize=13)

    for ax, diff in zip(axes, DIFFICULTIES):
        subset = df[df["Difficulty"] == diff]

        log_data = [
            np.log10(subset[subset["Config"] == cfg]["Time_s"].dropna().values)
            for cfg in configs
        ]
        valid = [(d, c) for d, c in zip(log_data, configs) if len(d) > 1]
        if not valid:
            ax.set_title(diff)
            continue
        valid_data, valid_configs = zip(*valid)

        parts = ax.violinplot(valid_data, positions=range(len(valid_configs)),
                              showmedians=True, showextrema=True)

        for body, cfg in zip(parts["bodies"], valid_configs):
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

        ax.set_xticks(range(len(valid_configs)))
        ax.set_xticklabels(valid_configs, rotation=40, ha="right", fontsize=7)
        ax.set_title(diff, fontsize=11)
        ax.set_ylabel("Time (seconds)" if diff == DIFFICULTIES[0] else "")

        lo, hi = ax.get_ylim()
        tick_pows = [p for p in range(-6, 2) if lo - 0.15 <= p <= hi + 0.15]
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
# PLOT 4 — Parallel Efficiency Heatmap (one per family)
# ============================================================
def plot_efficiency_heatmap(speedup_df):
    if not HAS_SEABORN:
        print("\n[Plot 4] Efficiency heatmap — SKIPPED (seaborn not installed)")
        return

    print("\n[Plot 4] Parallel Efficiency Heatmap")

    if speedup_df.empty:
        print("  Skipped — no speedup data.")
        return

    families = [f for f in ["Backtracking", "Backtracking-MRV", "Backtracking-Frontier", "DLX"]
                if f in speedup_df["Family"].values]
    subtitles = {
        "Backtracking": "Algorithm A — Backtracking",
        "Backtracking-MRV": "Algorithm A — Backtracking (MRV baseline)",
        "Backtracking-Frontier": "Algorithm A — Backtracking (Frontier)",
        "DLX": "Algorithm B — DLX",
    }

    fig, axes = plt.subplots(1, len(families), figsize=(6 * len(families), 4))
    if len(families) == 1:
        axes = [axes]

    for ax, family in zip(axes, families):
        agg = _aggregate_speedup(speedup_df, family=family)
        pivot = (
            agg.pivot(index="Difficulty", columns="Threads", values="agg_efficiency")
            .reindex(DIFFICULTIES)
        )
        sns.heatmap(pivot, annot=True, fmt=".2f", cmap="YlGn",
                    vmin=0, vmax=1.0, ax=ax, linewidths=0.5,
                    cbar_kws={"label": "Aggregate Efficiency  E = S_agg / P"})
        ax.set_title(subtitles[family])
        ax.set_xlabel("Threads")
        ax.set_ylabel("Difficulty")

    fig.suptitle("Parallel Efficiency by Difficulty and Thread Count  (ideal = 1.0)", fontsize=13)
    fig.tight_layout()
    path = os.path.join(PLOT_DIR, "efficiency_heatmap.png")
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved: {path}")


# ============================================================
# PLOT 5 — All Configurations Absolute Runtime (grouped bar)
# ============================================================
def plot_all_configs(df):
    print("\n[Plot 5] All Configurations Absolute Runtime")

    all_configs = (
            ["Serial", "Serial-MRV", "DLX"]
            + [f"OpenMP-{t}" for t in THREAD_COUNTS]
            + [f"OMP-Frontier-{t}" for t in THREAD_COUNTS]
            + [f"DLX-OMP-{t}" for t in THREAD_COUNTS]
    )
    configs = [c for c in all_configs if c in df["Config"].unique()]

    avg = df.groupby(["Difficulty", "Config"])["Time_s"].mean().reset_index()

    x = np.arange(len(DIFFICULTIES))
    n = len(configs)
    width = 0.85 / n

    fig, ax = plt.subplots(figsize=(13, 5))

    for i, cfg in enumerate(configs):
        vals = (
            avg[avg["Config"] == cfg]
            .set_index("Difficulty")
            .reindex(DIFFICULTIES)["Time_s"]
            .values
        )
        offset = (i - n / 2 + 0.5) * width
        ax.bar(x + offset, vals, width * 0.92,
               label=cfg, color=CONFIG_COLORS.get(cfg, "#888888"))

    ax.set_xlabel("Difficulty")
    ax.set_ylabel("Average Solve Time (seconds, log scale)")
    ax.set_title("Average Solve Time: All Algorithm Configurations")
    ax.set_xticks(x)
    ax.set_xticklabels(DIFFICULTIES)
    ax.set_yscale("log")
    ax.legend(ncol=2, fontsize=8)
    ax.grid(True, alpha=0.3, axis="y")

    path = os.path.join(PLOT_DIR, "all_configs_runtime.png")
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved: {path}")


# ============================================================
# PLOT 6 — Cost vs Threads (one subplot per family)
# ============================================================
def plot_cost_vs_threads(df):
    print("\n[Plot 6] Cost vs Threads")

    families = {
        "Backtracking": ("Serial", "OpenMP"),
        "Backtracking-MRV": ("Serial-MRV", "OpenMP"),
        "Backtracking-Frontier": ("Serial-MRV", "OMP-Frontier"),
        "DLX": ("DLX", "DLX-OMP"),
    }
    subtitles = {
        "Backtracking": "Algorithm A — Backtracking",
        "Backtracking-MRV": "Algorithm A — Backtracking (MRV baseline)",
        "Backtracking-Frontier": "Algorithm A — Backtracking (Frontier vs Serial-MRV)",
        "DLX": "Algorithm B — DLX",
    }

    fig, axes = plt.subplots(1, 4, figsize=(28, 5))

    for ax, (family, (s_alg, p_alg)) in zip(axes, families.items()):
        for diff in DIFFICULTIES:
            x_vals, y_vals = [], []

            s = df[(df["Algorithm"] == s_alg) & (df["Difficulty"] == diff)]
            if not s.empty:
                serial_cost = 1 * s["Time_s"].mean()
                x_vals.append(1)
                y_vals.append(serial_cost)
                ax.axhline(serial_cost, color=DIFF_COLORS[diff], lw=1, linestyle="--", alpha=0.4)

            for t in THREAD_COUNTS:
                p = df[(df["Algorithm"] == p_alg) & (df["Difficulty"] == diff) & (df["Threads"] == t)]
                if not p.empty:
                    x_vals.append(t)
                    y_vals.append(t * p["Time_s"].mean())

            if x_vals:
                ax.plot(x_vals, y_vals, marker="s", color=DIFF_COLORS[diff], label=diff, zorder=2)

        ax.set_xlabel("Threads  (1 = serial baseline)")
        ax.set_ylabel("Cost  C(P) = P × T(P)  (thread-seconds)")
        ax.set_title(subtitles[family])
        ax.set_xticks([1] + THREAD_COUNTS)
        ax.set_yscale("log")
        ax.grid(True, alpha=0.3, which="both")

        dash_proxy = mpatches.Patch(facecolor="none", edgecolor="gray",
                                    linestyle="--", label="— serial C(1) reference")
        handles, labels = ax.get_legend_handles_labels()
        ax.legend(handles=handles + [dash_proxy],
                  labels=labels + ["— serial C(1) reference"], fontsize=8)

    fig.suptitle("Parallel Cost vs Thread Count\n"
                 "Line ≈ dashed → cost-optimal  |  Rising line → growing overhead", fontsize=13)
    fig.tight_layout()
    path = os.path.join(PLOT_DIR, "cost_vs_threads.png")
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
        .agg(Time_median="median", Time_mean="mean", Time_std="std",
             Time_min="min", Time_max="max")
        .reset_index()
    )

    cost_agg = (
        df.groupby(["Difficulty", "Config"])["Cost"]
        .mean().reset_index()
        .rename(columns={"Cost": "Cost_mean"})
    )

    agg_rows = []
    if not speedup_df.empty:
        for (diff, config), g in speedup_df.groupby(["Difficulty", "Config"]):
            threads = g["Threads"].iloc[0]
            agg_speedup = g["T_serial"].sum() / g["Time_s"].sum()
            agg_rows.append({
                "Difficulty": diff,
                "Config": config,
                "Agg_Speedup": agg_speedup,
                "Agg_Efficiency": agg_speedup / threads,
            })
    agg_metrics = pd.DataFrame(agg_rows) if agg_rows else pd.DataFrame(
        columns=["Difficulty", "Config", "Agg_Speedup", "Agg_Efficiency"])

    mean_speedup = (
        speedup_df.groupby(["Difficulty", "Config"])["Speedup"]
        .mean().reset_index()
        .rename(columns={"Speedup": "Speedup_mean"})
    ) if not speedup_df.empty else pd.DataFrame(columns=["Difficulty", "Config", "Speedup_mean"])

    mean_efficiency = (
        speedup_df.groupby(["Difficulty", "Config"])["Efficiency"]
        .mean().reset_index()
        .rename(columns={"Efficiency": "Efficiency_mean"})
    ) if not speedup_df.empty else pd.DataFrame(columns=["Difficulty", "Config", "Efficiency_mean"])

    # Add speedup=1.0 reference rows for each serial baseline
    for s_config in ["Serial", "Serial-MRV", "DLX"]:
        if s_config not in time_agg["Config"].values:
            continue
        ref = time_agg[time_agg["Config"] == s_config][["Difficulty", "Config"]].copy()
        ref["Agg_Speedup"] = 1.0
        ref["Agg_Efficiency"] = 1.0
        agg_metrics = pd.concat([agg_metrics, ref], ignore_index=True)

        ref2 = ref[["Difficulty", "Config"]].copy()
        ref2["Speedup_mean"] = 1.0
        mean_speedup = pd.concat([mean_speedup, ref2], ignore_index=True)

        ref3 = ref[["Difficulty", "Config"]].copy()
        ref3["Efficiency_mean"] = 1.0
        mean_efficiency = pd.concat([mean_efficiency, ref3], ignore_index=True)

    summary = (
        time_agg
        .merge(cost_agg, on=["Difficulty", "Config"], how="left")
        .merge(agg_metrics, on=["Difficulty", "Config"], how="left")
        .merge(mean_speedup, on=["Difficulty", "Config"], how="left")
        .merge(mean_efficiency, on=["Difficulty", "Config"], how="left")
    )

    config_order = (
            ["Serial", "Serial-MRV"] + [f"OpenMP-{t}" for t in THREAD_COUNTS]
            + [f"OMP-Frontier-{t}" for t in THREAD_COUNTS]
            + ["DLX"] + [f"DLX-OMP-{t}" for t in THREAD_COUNTS]
    )
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
    import glob as _glob

    if len(sys.argv) < 2:
        print("Usage: python analyze.py results_*.csv")
        sys.exit(1)

    paths = []
    for arg in sys.argv[1:]:
        expanded = _glob.glob(arg)
        paths.extend(expanded if expanded else [arg])

    if not paths:
        print("No matching files found.")
        sys.exit(1)

    df = load_data(paths)
    df_correct = df[df["Correct"] == 1].copy()
    print(f"  {len(df_correct)} rows with Correct=1 used for timing plots.")

    speedup_df = compute_speedup(df_correct)
    if not speedup_df.empty:
        speedup_df["Config"] = speedup_df.apply(config_label, axis=1)

    plot_speedup_vs_threads(speedup_df)
    plot_runtime_vs_threads(df_correct)
    plot_runtime_distribution(df_correct)
    plot_efficiency_heatmap(speedup_df)
    plot_all_configs(df_correct)
    plot_cost_vs_threads(df_correct)
    save_summary_table(df_correct, speedup_df)

    print(f"\nAll plots written to: {PLOT_DIR}/")

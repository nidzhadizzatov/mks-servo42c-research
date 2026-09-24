#!/usr/bin/env python3
"""
Визуализация логов экспериментов MKS SERVO42C.

Использование:
    python plot_log.py <csv_file>

Пример:
    python plot_log.py ../data/logs/collision_test.csv
"""

import sys
import pandas as pd
import matplotlib.pyplot as plt


def main():
    if len(sys.argv) < 2:
        print("Usage: python plot_log.py <csv_file>")
        return 1

    csv_file = sys.argv[1]
    df = pd.read_csv(csv_file)

    required = {"timestamp_ms", "error_deg", "avg_error_deg", "shaft_status", "collision"}
    if not required.issubset(df.columns):
        print(f"Missing columns. Expected: {required}")
        print(f"Got: {list(df.columns)}")
        return 1

    t = df["timestamp_ms"] / 1000.0  # в секундах

    fig, axes = plt.subplots(3, 1, figsize=(12, 8), sharex=True)

    # График 1: error и avg_error
    axes[0].plot(t, df["error_deg"], label="error_deg", alpha=0.6, linewidth=0.8)
    axes[0].plot(t, df["avg_error_deg"], label="avg_error_deg", linewidth=1.5)
    axes[0].axhline(y=0.4, color="orange", linestyle="--", label="threshold (0.4°)")
    axes[0].set_ylabel("Error, deg")
    axes[0].legend()
    axes[0].grid(True, alpha=0.3)
    axes[0].set_title("Angle Error vs Time")

    # График 2: shaft_status
    axes[1].plot(t, df["shaft_status"], color="green", linewidth=1)
    axes[1].set_ylabel("Shaft status")
    axes[1].set_yticks([1, 2])
    axes[1].set_yticklabels(["BLOCKED", "free"])
    axes[1].grid(True, alpha=0.3)

    # График 3: collision flags
    axes[2].fill_between(t, 0, df["collision"], color="red", alpha=0.5,
                         label="collision")
    axes[2].set_ylabel("Collision")
    axes[2].set_xlabel("Time, s")
    axes[2].set_yticks([0, 1])
    axes[2].legend()
    axes[2].grid(True, alpha=0.3)

    # Метки столкновений на первом графике
    for i, row in df[df["collision"] == 1].iterrows():
        axes[0].axvline(x=row["timestamp_ms"] / 1000.0, color="red",
                        linestyle=":", alpha=0.5)

    plt.tight_layout()
    plt.savefig(csv_file.replace(".csv", ".png"), dpi=150)
    print(f"Plot saved: {csv_file.replace('.csv', '.png')}")
    plt.show()


if __name__ == "__main__":
    sys.exit(main())
#!/usr/bin/env python3
"""
Визуализация следящей ошибки MKS SERVO42C.

Использование:
    python plot_following_error.py <csv_file>
"""

import sys
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np


def main():
    if len(sys.argv) < 2:
        print("Usage: python plot_following_error.py <csv_file>")
        return 1

    csv_file = sys.argv[1]
    df = pd.read_csv(csv_file)

    required = {"speed", "avg_error", "max_error", "std_error"}
    if not required.issubset(df.columns):
        print(f"Missing columns. Expected: {required}")
        return 1

    fig, axes = plt.subplots(1, 2, figsize=(14, 6))

    # --- График 1: avg и max ошибка ---
    axes[0].plot(df["speed"], df["avg_error"], "o-",
                 color="blue", markersize=8, linewidth=2, label="avg |error|")
    axes[0].fill_between(df["speed"],
                         df["avg_error"] - df["std_error"],
                         df["avg_error"] + df["std_error"],
                         color="blue", alpha=0.2, label="±1 std")
    axes[0].plot(df["speed"], df["max_error"], "s--",
                 color="red", markersize=6, linewidth=1.5, label="max |error|")
    axes[0].set_xlabel("Speed (команда, 1–127)", fontsize=12)
    axes[0].set_ylabel("|angle_error|, deg", fontsize=12)
    axes[0].set_title("Следящая ошибка vs скорость (без нагрузки)", fontsize=13)
    axes[0].grid(True, alpha=0.3)
    axes[0].legend()

    # --- График 2: адаптивный порог ---
    # Предлагаем: threshold(speed) = base + k * speed/120
    base = df["avg_error"].iloc[0]
    max_avg = df["avg_error"].iloc[-1]
    k = max_avg - base

    suggested_threshold = base + k * (df["speed"] / 120.0)

    axes[1].plot(df["speed"], df["avg_error"], "o-",
                 color="blue", markersize=8, linewidth=2, label="avg |error|")
    axes[1].plot(df["speed"], suggested_threshold, "--",
                 color="green", linewidth=2, label="suggested threshold")
    axes[1].axhline(y=0.4, color="red", linestyle=":",
                    linewidth=2, label="fixed threshold (0.4°)")
    axes[1].set_xlabel("Speed (команда, 1–127)", fontsize=12)
    axes[1].set_ylabel("Error / Threshold, deg", fontsize=12)
    axes[1].set_title("Адаптивный порог для детектора столкновений", fontsize=13)
    axes[1].grid(True, alpha=0.3)
    axes[1].legend()

    plt.tight_layout()
    out_png = csv_file.replace(".csv", ".png")
    plt.savefig(out_png, dpi=150)
    print(f"Plot saved: {out_png}")

    # --- Таблица ---
    print("\n=== Following Error Table ===")
    print(df[["speed", "avg_error", "max_error", "std_error"]].to_string(index=False))

    # --- Анализ ---
    print("\n=== Analysis ===")
    print(f"At speed=10:  avg={df['avg_error'].iloc[0]:.4f}°  "
          f"max={df['max_error'].iloc[0]:.4f}°")
    print(f"At speed=60:  avg={df['avg_error'].iloc[len(df)//2]:.4f}°  "
          f"max={df['max_error'].iloc[len(df)//2]:.4f}°")
    print(f"At speed=120: avg={df['avg_error'].iloc[-1]:.4f}°  "
          f"max={df['max_error'].iloc[-1]:.4f}°")

    # Коэффициент роста
    ratio = df["avg_error"].iloc[-1] / df["avg_error"].iloc[0]
    print(f"\nGrowth factor (avg): {ratio:.2f}x")
    print(f"Growth factor (max): {df['max_error'].iloc[-1] / df['max_error'].iloc[0]:.2f}x")

    # Адаптивный порог
    print(f"\n=== Suggested Adaptive Threshold ===")
    print(f"threshold(speed) = {base:.4f} + {k:.4f} * (speed / 120)")
    print(f"Example threshold(60) = {base + k * 0.5:.4f}°")
    print(f"Example threshold(120) = {base + k:.4f}°")

    plt.show()


if __name__ == "__main__":
    sys.exit(main())
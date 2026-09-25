#!/usr/bin/env python3
"""
Визуализация калибровки скорости MKS SERVO42C.

Использование:
    python plot_calibration.py <csv_file>
"""

import sys
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np


def main():
    if len(sys.argv) < 2:
        print("Usage: python plot_calibration.py <csv_file>")
        return 1

    csv_file = sys.argv[1]
    df = pd.read_csv(csv_file)

    if "speed" not in df.columns or "rpm" not in df.columns:
        print("CSV must contain columns: speed, rpm")
        return 1

    # Отфильтровываем строки с нулевыми или отрицательными RPM
    df = df.copy()
    df["rpm"] = df["rpm"].abs()
    df = df[df["rpm"] > 0].reset_index(drop=True)
    if len(df) == 0:
        print("No valid data (all RPM <= 0)")
        return 1

    fig, axes = plt.subplots(1, 2, figsize=(14, 6))

    # --- График 1: RPM vs speed ---
    axes[0].plot(df["speed"], df["rpm"], "o-", color="blue",
                 markersize=8, linewidth=2, label="Measured")

    # Линейная зависимость от 0 до max speed для сравнения
    speed_max = df["speed"].max()
    rpm_max = df["rpm"].max()
    ideal = (df["speed"] / speed_max) * rpm_max
    axes[0].plot(df["speed"], ideal, "--", color="red", alpha=0.5,
                 label="Ideal (linear)")

    axes[0].set_xlabel("Speed (команда, 1–127)", fontsize=12)
    axes[0].set_ylabel("Real RPM", fontsize=12)
    axes[0].set_title("Speed → RPM: реальная зависимость vs идеальная", fontsize=13)
    axes[0].grid(True, alpha=0.3)
    axes[0].legend()

    # --- График 2: RPM / speed (эффективность) ---
    efficiency = df["rpm"] / df["speed"]
    axes[1].plot(df["speed"], efficiency, "s-", color="green",
                 markersize=8, linewidth=2)
    axes[1].set_xlabel("Speed (команда, 1–127)", fontsize=12)
    axes[1].set_ylabel("RPM / Speed", fontsize=12)
    axes[1].set_title("Эффективность: RPM на единицу команды", fontsize=13)
    axes[1].grid(True, alpha=0.3)

    plt.tight_layout()
    out_png = csv_file.replace(".csv", ".png")
    plt.savefig(out_png, dpi=150)
    print(f"Plot saved: {out_png}")

    # --- Таблица ---
    print("\n=== Calibration Table ===")
    print(df[["speed", "rpm"]].to_string(index=False))

    # --- Анализ ---
    print("\n=== Analysis ===")
    for _, row in df.iterrows():
        eff = row["rpm"] / row["speed"]
        print(f"speed={int(row['speed']):3d}  ->  RPM = {row['rpm']:7.2f}  "
              f"(eff = {eff:.3f})")

    # Нелинейность
    if len(df) > 2:
        ratio = df["rpm"].iloc[-1] / df["rpm"].iloc[0]
        speed_ratio = df["speed"].iloc[-1] / df["speed"].iloc[0]
        print(f"\nSpeed ratio (max/min): {speed_ratio:.2f}x")
        print(f"RPM ratio (max/min):   {ratio:.2f}x")
        print(f"Nonlinearity factor:   {ratio / speed_ratio:.2f}")

        if ratio / speed_ratio > 1.5:
            print("-> Сильная нелинейность (RPM растёт быстрее speed)")
        elif ratio / speed_ratio < 0.7:
            print("-> Обратная нелинейность (RPM растёт медленнее speed)")
        else:
            print("-> Умеренная нелинейность")

    plt.show()


if __name__ == "__main__":
    sys.exit(main())
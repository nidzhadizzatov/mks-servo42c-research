#!/usr/bin/env python3
"""
Анализ detection time.

Использование:
    python plot_detection_time.py <csv_file>
"""

import sys
import pandas as pd
import matplotlib.pyplot as plt


def main():
    if len(sys.argv) < 2:
        print("Usage: python plot_detection_time.py <csv_file>")
        return 1

    csv_file = sys.argv[1]
    df = pd.read_csv(csv_file)

    # Находим все события collision
    collisions = df[df["collision"] == 1]
    if collisions.empty:
        print("No collisions found in data")
        return 1

    print("=== Detection Time Analysis ===\n")
    print(f"Total samples: {len(df)}")
    print(f"Collision samples: {len(collisions)}")

    # Первое collision каждого события
    detection_times = []
    in_collision = False
    prev_time = 0

    for _, row in df.iterrows():
        if row["collision"] == 1 and not in_collision:
            t = row["timestamp_ms"]
            detection_times.append(t - prev_time)
            in_collision = True
        elif row["collision"] == 0:
            prev_time = row["timestamp_ms"]
            in_collision = False

    if detection_times:
        print(f"\nDetection times (ms):")
        for i, dt in enumerate(detection_times, 1):
            print(f"  Event #{i}: {dt:.0f} ms")

        print(f"\nStatistics:")
        print(f"  Mean:   {sum(detection_times)/len(detection_times):.0f} ms")
        print(f"  Min:    {min(detection_times):.0f} ms")
        print(f"  Max:    {max(detection_times):.0f} ms")

    # График
    fig, axes = plt.subplots(1, 2, figsize=(14, 6))

    # График 1: error и avg
    t = df["timestamp_ms"] / 1000.0
    axes[0].plot(t, df["error_deg"], label="error_deg", alpha=0.6)
    axes[0].plot(t, df["avg_deg"], label="avg_deg", linewidth=1.5)
    axes[0].axhline(y=0.15, color="orange", linestyle="--", label="threshold")

    # Метки столкновений
    for _, row in collisions.iterrows():
        axes[0].axvline(x=row["timestamp_ms"] / 1000.0,
                        color="red", linestyle=":", alpha=0.5)

    axes[0].set_xlabel("Time, s")
    axes[0].set_ylabel("Error, deg")
    axes[0].set_title("Detection Time: error vs time")
    axes[0].legend()
    axes[0].grid(True, alpha=0.3)

    # График 2: детекция
    axes[1].fill_between(t, 0, df["collision"], color="red", alpha=0.5)
    axes[1].set_xlabel("Time, s")
    axes[1].set_ylabel("Collision")
    axes[1].set_title("Detection: collision events")
    axes[1].set_yticks([0, 1])
    axes[1].grid(True, alpha=0.3)

    plt.tight_layout()
    out_png = csv_file.replace(".csv", ".png")
    plt.savefig(out_png, dpi=150)
    print(f"\nPlot saved: {out_png}")
    plt.show()


if __name__ == "__main__":
    sys.exit(main())
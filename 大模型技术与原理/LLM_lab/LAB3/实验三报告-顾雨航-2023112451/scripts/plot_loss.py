from __future__ import annotations

import argparse
import json
from pathlib import Path

import matplotlib


matplotlib.use("Agg")
import matplotlib.pyplot as plt


def parse_args():
    parser = argparse.ArgumentParser(description="Plot the training loss curve from trainer_state.json.")
    parser.add_argument(
        "--trainer_state",
        default="outputs/qwen2.5-7b-edu-lora/trainer_state.json",
        help="Path to trainer_state.json",
    )
    parser.add_argument(
        "--output_file",
        default="outputs/qwen2.5-7b-edu-lora/loss_curve.png",
        help="Path to save the loss figure.",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    trainer_state = Path(args.trainer_state)
    output_file = Path(args.output_file)
    output_file.parent.mkdir(parents=True, exist_ok=True)

    state = json.loads(trainer_state.read_text(encoding="utf-8"))
    loss_points = [
        (item["step"], item["loss"])
        for item in state.get("log_history", [])
        if "loss" in item and "step" in item
    ]
    if not loss_points:
        raise ValueError("No loss values were found in trainer_state.json.")

    steps = [point[0] for point in loss_points]
    losses = [point[1] for point in loss_points]

    plt.figure(figsize=(8, 5))
    plt.plot(steps, losses, marker="o", linewidth=1.8)
    plt.title("Training Loss Curve")
    plt.xlabel("Step")
    plt.ylabel("Loss")
    plt.grid(alpha=0.3)
    plt.tight_layout()
    plt.savefig(output_file, dpi=200)
    print(f"Saved loss curve to: {output_file}")


if __name__ == "__main__":
    main()

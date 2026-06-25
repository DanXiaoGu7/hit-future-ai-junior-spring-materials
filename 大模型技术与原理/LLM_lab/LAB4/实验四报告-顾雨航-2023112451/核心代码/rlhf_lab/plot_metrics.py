from __future__ import annotations

import argparse
from pathlib import Path

import matplotlib.pyplot as plt

from .utils import ensure_dir, load_json, load_jsonl


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Plot RM and PPO metrics for the report.")
    parser.add_argument("--rm-log", required=True, help="Path to RM log history JSON.")
    parser.add_argument("--ppo-log", required=True, help="Path to PPO metrics JSONL.")
    parser.add_argument("--output-dir", required=True, help="Directory where PNG files will be saved.")
    return parser.parse_args()


def plot_rm(log_history: list[dict], output_dir: Path) -> None:
    train_points = [(item["step"], item["loss"]) for item in log_history if "loss" in item and "step" in item]
    eval_points = [
        (item["step"], item["eval_accuracy"])
        for item in log_history
        if "eval_accuracy" in item and "step" in item
    ]

    if train_points:
        steps, losses = zip(*train_points)
        plt.figure()
        plt.plot(steps, losses)
        plt.xlabel("Step")
        plt.ylabel("Loss")
        plt.title("Reward Model Training Loss")
        plt.tight_layout()
        plt.savefig(output_dir / "rm_loss.png")
        plt.close()

    if eval_points:
        steps, accuracies = zip(*eval_points)
        plt.figure()
        plt.plot(steps, accuracies)
        plt.xlabel("Step")
        plt.ylabel("Accuracy")
        plt.title("Reward Model Validation Accuracy")
        plt.tight_layout()
        plt.savefig(output_dir / "rm_accuracy.png")
        plt.close()


def plot_ppo(metrics: list[dict], output_dir: Path) -> None:
    steps = [item["step"] for item in metrics]
    rewards = [item["mean_reward"] for item in metrics]
    kl_values = [item["objective/kl"] for item in metrics]

    if steps:
        plt.figure()
        plt.plot(steps, rewards)
        plt.xlabel("Step")
        plt.ylabel("Mean Reward")
        plt.title("PPO Mean Reward")
        plt.tight_layout()
        plt.savefig(output_dir / "ppo_reward.png")
        plt.close()

        plt.figure()
        plt.plot(steps, kl_values)
        plt.xlabel("Step")
        plt.ylabel("KL Divergence")
        plt.title("PPO KL Divergence")
        plt.tight_layout()
        plt.savefig(output_dir / "ppo_kl.png")
        plt.close()


def main() -> None:
    args = parse_args()
    output_dir = ensure_dir(args.output_dir)

    rm_log_history = load_json(args.rm_log)
    ppo_metrics = load_jsonl(args.ppo_log)

    plot_rm(rm_log_history, output_dir)
    plot_ppo(ppo_metrics, output_dir)

    print(f"Saved plots to {output_dir}")


if __name__ == "__main__":
    main()

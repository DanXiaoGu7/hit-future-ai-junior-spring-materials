"""
######################## train hopfield example ########################
Build a Hopfield associative memory on MNIST and save the checkpoint file:
python train.py --data_path ./data --ckpt_path ./ckpt
"""

import argparse
import json
import random
from pathlib import Path

import numpy as np
import torch

from src.config import mnist_cfg as cfg
from src.dataset import create_dataloader
from src.hopfield import HopfieldNetwork, build_memory_patterns, evaluate_model


def set_seed(seed):
    random.seed(seed)
    np.random.seed(seed)
    torch.manual_seed(seed)
    if torch.cuda.is_available():
        torch.cuda.manual_seed_all(seed)


def resolve_device(device_target):
    if device_target == "AUTO":
        return torch.device("cuda" if torch.cuda.is_available() else "cpu")
    if device_target == "GPU":
        if not torch.cuda.is_available():
            raise RuntimeError("GPU target was requested but CUDA is not available.")
        return torch.device("cuda")
    return torch.device("cpu")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="PyTorch Hopfield Example")
    parser.add_argument("--device_target", type=str, default=cfg.device_target, choices=["AUTO", "CPU", "GPU"],
                        help="device where the code will be implemented")
    parser.add_argument("--data_path", type=str, default="./data",
                        help="path where the dataset is saved")
    parser.add_argument("--ckpt_path", type=str, default="./ckpt",
                        help="directory where the checkpoint file will be saved")
    parser.add_argument("--batch_size", type=int, default=cfg.batch_size,
                        help="batch size used to read the training set")
    parser.add_argument("--eval_batch_size", type=int, default=cfg.eval_batch_size,
                        help="batch size used during evaluation")
    parser.add_argument("--num_workers", type=int, default=cfg.num_workers,
                        help="number of dataloader workers")
    parser.add_argument("--num_memories_per_class", type=int, default=cfg.num_memories_per_class,
                        help="number of stored memory patterns for each class")
    parser.add_argument("--kmeans_iters", type=int, default=cfg.kmeans_iters,
                        help="number of k-means iterations used to build memory patterns")
    parser.add_argument("--threshold", type=float, default=cfg.threshold,
                        help="threshold used to binarize images into bipolar patterns")
    parser.add_argument("--recall_steps", type=int, default=cfg.recall_steps,
                        help="maximum synchronous update steps during recall")
    parser.add_argument("--seed", type=int, default=cfg.seed,
                        help="random seed")

    args = parser.parse_args()

    set_seed(args.seed)
    device = resolve_device(args.device_target)

    data_path = Path(args.data_path)
    ckpt_path = Path(args.ckpt_path)
    ckpt_path.mkdir(parents=True, exist_ok=True)

    train_loader = create_dataloader(
        data_path=data_path,
        batch_size=args.batch_size,
        train=True,
        download=True,
        num_workers=args.num_workers,
    )
    test_loader = create_dataloader(
        data_path=data_path,
        batch_size=args.eval_batch_size,
        train=False,
        download=True,
        num_workers=args.num_workers,
    )

    print("============== Building Hopfield Memories ==============")
    print(f"device: {device}")
    print(f"num_memories_per_class: {args.num_memories_per_class}")

    memory_patterns, memory_labels = build_memory_patterns(
        dataloader=train_loader,
        num_classes=cfg.num_classes,
        num_memories_per_class=args.num_memories_per_class,
        threshold=args.threshold,
        kmeans_iters=args.kmeans_iters,
        seed=args.seed,
    )

    network = HopfieldNetwork(num_units=cfg.image_height * cfg.image_width, device=device)
    network.fit(memory_patterns.to(device))

    accuracy, avg_steps = evaluate_model(
        network=network,
        dataloader=test_loader,
        memory_patterns=memory_patterns.to(device),
        memory_labels=memory_labels.to(device),
        threshold=args.threshold,
        recall_steps=args.recall_steps,
        device=device,
    )

    ckpt_file = ckpt_path / "hopfield_mnist.pt"
    summary_file = ckpt_path / "summary.json"

    torch.save(
        {
            "weight_matrix": network.weight_matrix.cpu(),
            "memory_patterns": memory_patterns.cpu(),
            "memory_labels": memory_labels.cpu(),
            "config": {
                "num_classes": cfg.num_classes,
                "image_height": cfg.image_height,
                "image_width": cfg.image_width,
                "num_memories_per_class": args.num_memories_per_class,
                "kmeans_iters": args.kmeans_iters,
                "threshold": args.threshold,
                "recall_steps": args.recall_steps,
                "seed": args.seed,
            },
        },
        ckpt_file,
    )

    summary = {
        "num_units": cfg.image_height * cfg.image_width,
        "num_memory_patterns": int(memory_patterns.size(0)),
        "num_memories_per_class": args.num_memories_per_class,
        "threshold": args.threshold,
        "matching_mode": "nearest_memory",
        "recall_steps": args.recall_steps,
        "test_accuracy": round(accuracy, 6),
        "average_recall_steps": round(avg_steps, 4),
    }
    summary_file.write_text(json.dumps(summary, indent=2), encoding="utf-8")

    print("============== Training Finished ==============")
    print(f"checkpoint: {ckpt_file}")
    print(f"summary: {summary_file}")
    print("matching_mode: nearest_memory")
    print(f"test_accuracy: {accuracy * 100:.2f}%")
    print(f"average_recall_steps: {avg_steps:.2f}")

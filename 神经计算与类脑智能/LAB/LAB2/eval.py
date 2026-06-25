"""
######################## eval hopfield example ########################
Evaluate a Hopfield network on MNIST according to the checkpoint file:
python eval.py --data_path ./data --ckpt_path ./ckpt/hopfield_mnist.pt
"""

import argparse
from pathlib import Path

import torch

from src.config import mnist_cfg as cfg
from src.dataset import create_dataloader
from src.hopfield import HopfieldNetwork, evaluate_model


def resolve_device(device_target):
    if device_target == "AUTO":
        return torch.device("cuda" if torch.cuda.is_available() else "cpu")
    if device_target == "GPU":
        if not torch.cuda.is_available():
            raise RuntimeError("GPU target was requested but CUDA is not available.")
        return torch.device("cuda")
    return torch.device("cpu")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="PyTorch Hopfield Evaluation")
    parser.add_argument("--device_target", type=str, default=cfg.device_target, choices=["AUTO", "CPU", "GPU"],
                        help="device where the code will be implemented")
    parser.add_argument("--data_path", type=str, default="./data",
                        help="path where the dataset is saved")
    parser.add_argument("--ckpt_path", type=str, required=True,
                        help="path to the trained checkpoint file")
    parser.add_argument("--batch_size", type=int, default=cfg.eval_batch_size,
                        help="evaluation batch size")
    parser.add_argument("--num_workers", type=int, default=cfg.num_workers,
                        help="number of dataloader workers")

    args = parser.parse_args()

    device = resolve_device(args.device_target)
    checkpoint_file = Path(args.ckpt_path)
    if not checkpoint_file.exists():
        raise FileNotFoundError(f"Checkpoint file not found: {checkpoint_file}")

    checkpoint = torch.load(checkpoint_file, map_location=device)
    config = checkpoint["config"]
    memory_patterns = checkpoint["memory_patterns"].to(device)
    memory_labels = checkpoint["memory_labels"].to(device)

    network = HopfieldNetwork(
        num_units=int(memory_patterns.size(1)),
        device=device,
    )
    network.load_weights(checkpoint["weight_matrix"].to(device))

    test_loader = create_dataloader(
        data_path=Path(args.data_path),
        batch_size=args.batch_size,
        train=False,
        download=True,
        num_workers=args.num_workers,
    )

    accuracy, avg_steps = evaluate_model(
        network=network,
        dataloader=test_loader,
        memory_patterns=memory_patterns,
        memory_labels=memory_labels,
        threshold=config["threshold"],
        recall_steps=config["recall_steps"],
        device=device,
    )

    print("============== Starting Evaluation ==============")
    print(f"device: {device}")
    print(f"checkpoint: {checkpoint_file}")
    print(f"num_memory_patterns: {memory_patterns.size(0)}")
    print("matching_mode: nearest_memory")
    print(f"test_accuracy: {accuracy * 100:.2f}%")
    print(f"average_recall_steps: {avg_steps:.2f}")

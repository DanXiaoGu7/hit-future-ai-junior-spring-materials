"""
######################## eval mlp example ########################
Evaluate an MLP on MNIST according to the checkpoint file:
python eval.py --data_path ./data --ckpt_path ./ckpt/best_mlp_mnist.pt
"""

import argparse
from pathlib import Path

import torch
import torch.nn as nn

from src.config import mnist_cfg as cfg
from src.dataset import create_dataloader
from src.mlp import MLP


def resolve_device(device_target):
    if device_target == "AUTO":
        return torch.device("cuda" if torch.cuda.is_available() else "cpu")
    if device_target == "GPU":
        if not torch.cuda.is_available():
            raise RuntimeError("GPU target was requested but CUDA is not available.")
        return torch.device("cuda")
    return torch.device("cpu")


@torch.no_grad()
def evaluate(model, dataloader, criterion, device):
    model.eval()
    total_loss = 0.0
    total_correct = 0
    total_samples = 0

    for data, target in dataloader:
        data = data.to(device)
        target = target.to(device)

        output = model(data)
        loss = criterion(output, target)

        batch_size = data.size(0)
        total_loss += loss.item() * batch_size
        total_correct += output.argmax(dim=1).eq(target).sum().item()
        total_samples += batch_size

    return total_loss / total_samples, total_correct / total_samples


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="PyTorch MLP Evaluation")
    parser.add_argument("--device_target", type=str, default=cfg.device_target, choices=["AUTO", "CPU", "GPU"],
                        help="device where the code will be implemented")
    parser.add_argument("--data_path", type=str, default="./data",
                        help="path where the dataset is saved")
    parser.add_argument("--ckpt_path", type=str, required=True,
                        help="path to the trained checkpoint file")
    parser.add_argument("--batch_size", type=int, default=cfg.test_batch_size,
                        help="evaluation batch size")
    parser.add_argument("--num_workers", type=int, default=cfg.num_workers,
                        help="number of dataloader workers")

    args = parser.parse_args()

    device = resolve_device(args.device_target)
    checkpoint_file = Path(args.ckpt_path)

    if not checkpoint_file.exists():
        raise FileNotFoundError(f"Checkpoint file not found: {checkpoint_file}")

    checkpoint = torch.load(checkpoint_file, map_location=device)
    model_cfg = checkpoint.get("config", {})
    network = MLP(
        hidden_dim_1=model_cfg.get("hidden_dim_1", cfg.hidden_dim_1),
        hidden_dim_2=model_cfg.get("hidden_dim_2", cfg.hidden_dim_2),
        dropout=model_cfg.get("dropout", cfg.dropout),
    ).to(device)
    network.load_state_dict(checkpoint["model_state_dict"])

    test_loader = create_dataloader(
        data_path=Path(args.data_path),
        batch_size=args.batch_size,
        train=False,
        download=True,
        num_workers=args.num_workers,
    )
    criterion = nn.CrossEntropyLoss()
    test_loss, test_accuracy = evaluate(network, test_loader, criterion, device)

    print("============== Starting Evaluation ==============")
    print(f"device: {device}")
    print(f"checkpoint: {checkpoint_file}")
    print(f"test_loss: {test_loss:.6f}")
    print(f"test_accuracy: {test_accuracy * 100:.2f}%")

"""
######################## train mlp example ########################
Train an MLP on MNIST and save the checkpoint file:
python train.py --data_path ./data --ckpt_path ./ckpt
"""

import argparse
import json
import random
from pathlib import Path

import torch
import torch.nn as nn
import torch.optim as optim

from src.config import mnist_cfg as cfg
from src.dataset import create_dataloader
from src.mlp import MLP


def set_seed(seed):
    random.seed(seed)
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


def build_optimizer(name, model, lr, momentum):
    if name == "adam":
        return optim.Adam(model.parameters(), lr=lr)
    return optim.SGD(model.parameters(), lr=lr, momentum=momentum)


def train_one_epoch(model, dataloader, criterion, optimizer, device, epoch, log_interval):
    model.train()
    running_loss = 0.0
    total_samples = 0

    for batch_idx, (data, target) in enumerate(dataloader, start=1):
        data = data.to(device)
        target = target.to(device)

        optimizer.zero_grad()
        output = model(data)
        loss = criterion(output, target)
        loss.backward()
        optimizer.step()

        batch_size = data.size(0)
        running_loss += loss.item() * batch_size
        total_samples += batch_size

        if batch_idx % log_interval == 0 or batch_idx == len(dataloader):
            processed = min(batch_idx * batch_size, len(dataloader.dataset))
            progress = 100.0 * batch_idx / len(dataloader)
            print(
                f"epoch: {epoch:02d} step: {batch_idx:04d}/{len(dataloader):04d}, "
                f"samples: {processed:05d}/{len(dataloader.dataset):05d}, "
                f"progress: {progress:6.2f}%, loss: {loss.item():.6f}"
            )

    return running_loss / total_samples


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


def save_history(history, history_path):
    history_path.write_text(json.dumps(history, indent=2), encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="PyTorch MLP Example")
    parser.add_argument("--device_target", type=str, default=cfg.device_target, choices=["AUTO", "CPU", "GPU"],
                        help="device where the code will be implemented")
    parser.add_argument("--data_path", type=str, default="./data",
                        help="path where the dataset is saved")
    parser.add_argument("--ckpt_path", type=str, default="./ckpt",
                        help="directory where the checkpoint file will be saved")
    parser.add_argument("--batch_size", type=int, default=cfg.batch_size,
                        help="training batch size")
    parser.add_argument("--test_batch_size", type=int, default=cfg.test_batch_size,
                        help="test batch size")
    parser.add_argument("--epoch_size", type=int, default=cfg.epoch_size,
                        help="total training epochs")
    parser.add_argument("--lr", type=float, default=cfg.lr,
                        help="learning rate")
    parser.add_argument("--momentum", type=float, default=cfg.momentum,
                        help="momentum for SGD optimizer")
    parser.add_argument("--optimizer", type=str, default=cfg.optimizer, choices=["sgd", "adam"],
                        help="optimizer type")
    parser.add_argument("--num_workers", type=int, default=cfg.num_workers,
                        help="number of dataloader workers")
    parser.add_argument("--log_interval", type=int, default=cfg.log_interval,
                        help="logging interval in training")
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
        batch_size=args.test_batch_size,
        train=False,
        download=True,
        num_workers=args.num_workers,
    )

    network = MLP(
        hidden_dim=cfg.hidden_dim,
        dropout=cfg.dropout,
    ).to(device)
    criterion = nn.CrossEntropyLoss()
    optimizer = build_optimizer(args.optimizer, network, args.lr, args.momentum)

    best_accuracy = 0.0
    history = []
    best_ckpt_file = ckpt_path / "best_mlp_one_hidden_mnist.pt"
    history_file = ckpt_path / "history.json"

    print("============== Starting Training ==============")
    print(f"device: {device}")

    for epoch in range(1, args.epoch_size + 1):
        train_loss = train_one_epoch(
            model=network,
            dataloader=train_loader,
            criterion=criterion,
            optimizer=optimizer,
            device=device,
            epoch=epoch,
            log_interval=args.log_interval,
        )
        test_loss, test_accuracy = evaluate(
            model=network,
            dataloader=test_loader,
            criterion=criterion,
            device=device,
        )

        epoch_result = {
            "epoch": epoch,
            "train_loss": round(train_loss, 6),
            "test_loss": round(test_loss, 6),
            "test_accuracy": round(test_accuracy, 6),
        }
        history.append(epoch_result)
        save_history(history, history_file)

        print(
            f"Epoch {epoch:02d} finished | "
            f"train_loss: {train_loss:.6f} | "
            f"test_loss: {test_loss:.6f} | "
            f"test_accuracy: {test_accuracy * 100:.2f}%"
        )

        if test_accuracy > best_accuracy:
            best_accuracy = test_accuracy
            torch.save(
                {
                    "epoch": epoch,
                    "model_state_dict": network.state_dict(),
                    "optimizer_state_dict": optimizer.state_dict(),
                    "test_accuracy": test_accuracy,
                    "config": {
                        "hidden_dim": cfg.hidden_dim,
                        "dropout": cfg.dropout,
                    },
                },
                best_ckpt_file,
            )
            print(f"Saved best checkpoint to: {best_ckpt_file}")

    print("============== Training Finished ==============")
    print(f"Best Accuracy: {best_accuracy * 100:.2f}%")
    print(f"Checkpoint Path: {best_ckpt_file}")
    print(f"History Path: {history_file}")

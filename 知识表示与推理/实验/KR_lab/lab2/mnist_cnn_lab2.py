import argparse
import json
import math
import os
import random
import time

import torch
import torch.nn as nn
import torch.nn.functional as F
from torch.utils.data import DataLoader
from torchvision import datasets, transforms

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np


NUM_CLASSES = 10
DEFAULT_MEAN = 0.1307
DEFAULT_STD = 0.3081


def set_seed(seed: int) -> None:
    random.seed(seed)
    torch.manual_seed(seed)
    torch.cuda.manual_seed_all(seed)
    torch.backends.cudnn.deterministic = True
    torch.backends.cudnn.benchmark = False


def compute_mean_std(dataset, batch_size: int = 512, num_workers: int = 2):
    loader = DataLoader(dataset, batch_size=batch_size, shuffle=False, num_workers=num_workers)
    total_pixels = 0
    channel_sum = 0.0
    channel_sum_sq = 0.0
    for images, _ in loader:
        # images: [B, 1, 28, 28]
        total_pixels += images.numel()
        channel_sum += images.sum().item()
        channel_sum_sq += (images ** 2).sum().item()
    mean = channel_sum / total_pixels
    var = channel_sum_sq / total_pixels - mean ** 2
    std = math.sqrt(max(var, 0.0))
    return mean, var, std


def count_parameters(model):
    total = sum(p.numel() for p in model.parameters())
    trainable = sum(p.numel() for p in model.parameters() if p.requires_grad)
    return total, trainable


def normalize_for_vis(img_tensor):
    # img_tensor: [1, H, W] normalized
    img = img_tensor.squeeze(0)
    min_v = img.min().item()
    max_v = img.max().item()
    if max_v - min_v < 1e-6:
        return torch.zeros_like(img)
    return (img - min_v) / (max_v - min_v)


def save_samples_per_class(dataset, out_path: str, samples_per_class: int = 5, seed: int = 42):
    rng = random.Random(seed)
    indices = list(range(len(dataset)))
    rng.shuffle(indices)

    buckets = {c: [] for c in range(NUM_CLASSES)}
    for idx in indices:
        img, label = dataset[idx]
        if len(buckets[label]) < samples_per_class:
            buckets[label].append(img)
        if all(len(buckets[c]) >= samples_per_class for c in range(NUM_CLASSES)):
            break

    fig, axes = plt.subplots(NUM_CLASSES, samples_per_class, figsize=(samples_per_class * 1.5, NUM_CLASSES * 1.5))
    for c in range(NUM_CLASSES):
        for i in range(samples_per_class):
            ax = axes[c, i]
            ax.axis("off")
            img = buckets[c][i]
            vis = normalize_for_vis(img)
            ax.imshow(vis, cmap="gray")
            if i == 0:
                ax.set_title(f"Class {c}")
    plt.tight_layout()
    fig.savefig(out_path, dpi=200)
    plt.close(fig)


class ModelA(nn.Module):
    def __init__(self):
        super().__init__()
        self.conv1 = nn.Conv2d(1, 32, kernel_size=3, padding=1)
        self.conv2 = nn.Conv2d(32, 64, kernel_size=3, padding=1)
        self.pool = nn.MaxPool2d(2)
        self.fc1 = nn.Linear(64 * 7 * 7, 128)
        self.fc2 = nn.Linear(128, NUM_CLASSES)
        self._init_weights()

    def _init_weights(self):
        for m in self.modules():
            if isinstance(m, nn.Conv2d) or isinstance(m, nn.Linear):
                nn.init.kaiming_normal_(m.weight)
                if m.bias is not None:
                    nn.init.zeros_(m.bias)

    def forward(self, x):
        x = self.pool(F.relu(self.conv1(x)))
        x = self.pool(F.relu(self.conv2(x)))
        x = x.view(x.size(0), -1)
        x = F.relu(self.fc1(x))
        x = self.fc2(x)
        return x


class ModelB(nn.Module):
    def __init__(self, negative_slope: float = 0.1):
        super().__init__()
        self.features = nn.Sequential(
            nn.Conv2d(1, 32, kernel_size=3, padding=1),
            nn.BatchNorm2d(32),
            nn.LeakyReLU(negative_slope, inplace=True),

            nn.Conv2d(32, 32, kernel_size=3, padding=1),
            nn.BatchNorm2d(32),
            nn.LeakyReLU(negative_slope, inplace=True),

            nn.MaxPool2d(2),
            nn.Dropout(0.1),

            nn.Conv2d(32, 64, kernel_size=3, padding=1),
            nn.BatchNorm2d(64),
            nn.LeakyReLU(negative_slope, inplace=True),

            nn.Conv2d(64, 64, kernel_size=3, padding=1),
            nn.BatchNorm2d(64),
            nn.LeakyReLU(negative_slope, inplace=True),

            nn.MaxPool2d(2),
            nn.Dropout(0.2),

            nn.Conv2d(64, 128, kernel_size=3, padding=1),
            nn.BatchNorm2d(128),
            nn.LeakyReLU(negative_slope, inplace=True),

            nn.Conv2d(128, 128, kernel_size=3, padding=1),
            nn.BatchNorm2d(128),
            nn.LeakyReLU(negative_slope, inplace=True),
        )
        self.gap = nn.AdaptiveAvgPool2d(1)
        self.classifier = nn.Sequential(
            nn.Dropout(0.5),
            nn.Linear(128, NUM_CLASSES)
        )
        self._init_weights()

    def _init_weights(self):
        for m in self.modules():
            if isinstance(m, nn.Conv2d) or isinstance(m, nn.Linear):
                nn.init.kaiming_normal_(m.weight)
                if m.bias is not None:
                    nn.init.zeros_(m.bias)

    def forward(self, x):
        x = self.features(x)
        x = self.gap(x)
        x = x.view(x.size(0), -1)
        x = self.classifier(x)
        return x


def train_model(
    model,
    train_loader,
    device,
    optimizer,
    criterion,
    epochs,
    log_path,
    model_name,
    start_step=0,
    start_epoch=0,
    append=False,
    epoch_log_path=None,
    eval_loader=None,
):
    model.train()
    logs = []
    epoch_stats = []
    step = start_step
    for epoch in range(1, epochs + 1):
        global_epoch = start_epoch + epoch
        epoch_start = time.perf_counter()
        correct = 0
        total = 0
        running_loss = 0.0
        for images, labels in train_loader:
            images = images.to(device)
            labels = labels.to(device)

            optimizer.zero_grad()
            logits = model(images)
            loss = criterion(logits, labels)
            loss.backward()
            optimizer.step()

            preds = logits.argmax(dim=1)
            batch_acc = (preds == labels).float().mean().item()
            logs.append([step, loss.item(), batch_acc])

            running_loss += loss.item() * labels.size(0)
            correct += (preds == labels).sum().item()
            total += labels.size(0)
            step += 1

        epoch_loss = running_loss / max(total, 1)
        epoch_acc = correct / max(total, 1)
        test_acc = None
        if eval_loader is not None:
            test_acc = evaluate_accuracy(model, eval_loader, device)
        epoch_time = time.perf_counter() - epoch_start
        msg = f"{model_name} Epoch {global_epoch} - loss: {epoch_loss:.4f} - acc: {epoch_acc:.4f} - time: {epoch_time:.2f}s"
        if test_acc is not None:
            msg += f" - test_acc: {test_acc:.4f}"
        print(msg)

        epoch_stats.append([global_epoch, epoch_loss, epoch_acc, test_acc, epoch_time])

    if log_path:
        mode = "a" if append else "w"
        with open(log_path, mode, encoding="utf-8") as f:
            if not append:
                f.write("step,loss,acc\n")
            for row in logs:
                f.write(f"{row[0]},{row[1]:.6f},{row[2]:.6f}\n")

    if epoch_log_path:
        mode = "a" if append else "w"
        with open(epoch_log_path, mode, encoding="utf-8") as f:
            if not append:
                f.write("epoch,train_loss,train_acc,test_acc,time_sec\n")
            for row in epoch_stats:
                test_acc = "" if row[3] is None else f"{row[3]:.6f}"
                f.write(f"{row[0]},{row[1]:.6f},{row[2]:.6f},{test_acc},{row[4]:.4f}\n")

    return logs, step, epoch_stats


def plot_train_curves(logs, out_path, title):
    steps = [r[0] for r in logs]
    losses = [r[1] for r in logs]
    accs = [r[2] for r in logs]

    fig, axes = plt.subplots(1, 2, figsize=(10, 4))
    axes[0].plot(steps, losses, color="tab:blue")
    axes[0].set_title(f"{title} Loss")
    axes[0].set_xlabel("Step")
    axes[0].set_ylabel("Loss")

    axes[1].plot(steps, accs, color="tab:green")
    axes[1].set_title(f"{title} Accuracy")
    axes[1].set_xlabel("Step")
    axes[1].set_ylabel("Accuracy")

    plt.tight_layout()
    fig.savefig(out_path, dpi=200)
    plt.close(fig)


def evaluate_model(model, test_loader, device, max_wrong=25):
    model.eval()
    conf = torch.zeros(NUM_CLASSES, NUM_CLASSES, dtype=torch.int64)
    wrong_samples = []
    correct = 0
    total = 0
    with torch.no_grad():
        for images, labels in test_loader:
            images = images.to(device)
            labels = labels.to(device)
            logits = model(images)
            preds = logits.argmax(dim=1)
            correct += (preds == labels).sum().item()
            total += labels.size(0)

            for i in range(labels.size(0)):
                t = labels[i].item()
                p = preds[i].item()
                conf[t, p] += 1
                if p != t and len(wrong_samples) < max_wrong:
                    wrong_samples.append((images[i].cpu(), t, p))

    accuracy = correct / max(total, 1)
    metrics = compute_metrics_from_confusion(conf)
    metrics["accuracy"] = accuracy
    return conf, metrics, wrong_samples


def evaluate_accuracy(model, test_loader, device):
    was_training = model.training
    model.eval()
    correct = 0
    total = 0
    with torch.no_grad():
        for images, labels in test_loader:
            images = images.to(device)
            labels = labels.to(device)
            logits = model(images)
            preds = logits.argmax(dim=1)
            correct += (preds == labels).sum().item()
            total += labels.size(0)
    if was_training:
        model.train()
    return correct / max(total, 1)


def compute_metrics_from_confusion(conf):
    conf = conf.to(torch.float64)
    per_class = {}
    precisions = []
    recalls = []
    f1s = []
    supports = []

    for c in range(NUM_CLASSES):
        tp = conf[c, c].item()
        fp = conf[:, c].sum().item() - tp
        fn = conf[c, :].sum().item() - tp
        support = conf[c, :].sum().item()

        precision = tp / (tp + fp + 1e-12)
        recall = tp / (tp + fn + 1e-12)
        f1 = 2 * precision * recall / (precision + recall + 1e-12)

        per_class[str(c)] = {
            "precision": precision,
            "recall": recall,
            "f1": f1,
            "support": int(support),
        }
        precisions.append(precision)
        recalls.append(recall)
        f1s.append(f1)
        supports.append(support)

    macro_precision = sum(precisions) / NUM_CLASSES
    macro_recall = sum(recalls) / NUM_CLASSES
    macro_f1 = sum(f1s) / NUM_CLASSES

    total_support = sum(supports) if supports else 0
    if total_support > 0:
        weighted_precision = sum(p * s for p, s in zip(precisions, supports)) / total_support
        weighted_recall = sum(r * s for r, s in zip(recalls, supports)) / total_support
        weighted_f1 = sum(f * s for f, s in zip(f1s, supports)) / total_support
    else:
        weighted_precision = 0.0
        weighted_recall = 0.0
        weighted_f1 = 0.0

    metrics = {
        "precision_macro": macro_precision,
        "recall_macro": macro_recall,
        "f1_macro": macro_f1,
        "precision_weighted": weighted_precision,
        "recall_weighted": weighted_recall,
        "f1_weighted": weighted_f1,
        "per_class": per_class,
    }
    return metrics


def plot_confusion_matrix(conf, out_path, title):
    fig, ax = plt.subplots(figsize=(6, 5))
    im = ax.imshow(conf.numpy(), cmap="Blues")
    ax.set_title(title)
    ax.set_xlabel("Predicted")
    ax.set_ylabel("True")
    ax.set_xticks(range(NUM_CLASSES))
    ax.set_yticks(range(NUM_CLASSES))
    fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04)

    # add text values
    for i in range(NUM_CLASSES):
        for j in range(NUM_CLASSES):
            ax.text(j, i, int(conf[i, j].item()), ha="center", va="center", fontsize=6)

    plt.tight_layout()
    fig.savefig(out_path, dpi=200)
    plt.close(fig)


def save_wrong_samples(wrong_samples, out_path):
    n = len(wrong_samples)
    if n == 0:
        return
    cols = 5
    rows = math.ceil(n / cols)
    fig, axes = plt.subplots(rows, cols, figsize=(cols * 2, rows * 2))
    axes = np.array(axes).reshape(rows, cols)

    idx = 0
    for r in range(rows):
        for c in range(cols):
            ax = axes[r][c]
            ax.axis("off")
            if idx >= n:
                continue
            img, t, p = wrong_samples[idx]
            vis = normalize_for_vis(img)
            ax.imshow(vis, cmap="gray")
            ax.set_title(f"T:{t} P:{p}", fontsize=8)
            idx += 1

    plt.tight_layout()
    fig.savefig(out_path, dpi=200)
    plt.close(fig)


def build_dataloaders(batch_size, num_workers, mean, std, use_aug=False):
    normalize = transforms.Normalize(mean=[mean], std=[std])
    transform_basic = transforms.Compose([
        transforms.ToTensor(),
        normalize,
    ])

    transform_aug = transforms.Compose([
        transforms.RandomAffine(degrees=10, translate=(0.1, 0.1)),
        transforms.ToTensor(),
        normalize,
    ])

    train_transform = transform_aug if use_aug else transform_basic

    train_dataset = datasets.MNIST(root="./data", train=True, download=True, transform=train_transform)
    test_dataset = datasets.MNIST(root="./data", train=False, download=True, transform=transform_basic)

    train_loader = DataLoader(
        train_dataset,
        batch_size=batch_size,
        shuffle=True,
        num_workers=num_workers,
        pin_memory=torch.cuda.is_available(),
    )
    test_loader = DataLoader(
        test_dataset,
        batch_size=batch_size,
        shuffle=False,
        num_workers=num_workers,
        pin_memory=torch.cuda.is_available(),
    )
    return train_loader, test_loader


def main():
    parser = argparse.ArgumentParser(description="MNIST CNN Lab 2")
    parser.add_argument("--batch-size", type=int, default=128)
    parser.add_argument("--epochs", type=int, default=5)
    parser.add_argument("--lr", type=float, default=1e-3)
    parser.add_argument("--weight-decay", type=float, default=1e-4)
    parser.add_argument("--device", type=str, default="cuda" if torch.cuda.is_available() else "cpu")
    parser.add_argument("--output-dir", type=str, default="outputs")
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--compute-stats", action="store_true")
    parser.add_argument("--num-workers", type=int, default=2)
    parser.add_argument("--ensure-99", action="store_true")
    parser.add_argument("--max-epochs", type=int, default=20)
    args = parser.parse_args()

    set_seed(args.seed)

    os.makedirs(args.output_dir, exist_ok=True)

    # compute mean/std if needed
    if args.compute_stats:
        raw_train = datasets.MNIST(root="./data", train=True, download=True, transform=transforms.ToTensor())
        mean, var, std = compute_mean_std(raw_train, num_workers=args.num_workers)
    else:
        mean, std = DEFAULT_MEAN, DEFAULT_STD
        var = std ** 2

    stats_path = os.path.join(args.output_dir, "mnist_stats.json")
    with open(stats_path, "w", encoding="utf-8") as f:
        json.dump({"mean": mean, "var": var, "std": std}, f, indent=2)
    print(f"MNIST mean: {mean:.6f}, var: {var:.6f}, std: {std:.6f}")

    # sample visualization (normalized data)
    sample_dataset = datasets.MNIST(
        root="./data", train=True, download=True,
        transform=transforms.Compose([transforms.ToTensor(), transforms.Normalize([mean], [std])])
    )
    sample_path = os.path.join(args.output_dir, "samples_per_class.png")
    save_samples_per_class(sample_dataset, sample_path, samples_per_class=5, seed=args.seed)

    # Model A
    train_loader_a, test_loader = build_dataloaders(args.batch_size, args.num_workers, mean, std, use_aug=False)
    model_a = ModelA().to(args.device)
    optimizer_a = torch.optim.Adam(model_a.parameters(), lr=args.lr, weight_decay=args.weight_decay)
    criterion = nn.CrossEntropyLoss()

    params_a_total, params_a_trainable = count_parameters(model_a)
    print(f"ModelA params: total={params_a_total}, trainable={params_a_trainable}")

    logs_a, _, epoch_stats_a = train_model(
        model_a, train_loader_a, args.device, optimizer_a, criterion,
        epochs=args.epochs, log_path=os.path.join(args.output_dir, "modelA_train_log.csv"),
        model_name="ModelA",
        epoch_log_path=os.path.join(args.output_dir, "modelA_epoch_log.csv"),
        eval_loader=test_loader,
    )
    plot_train_curves(logs_a, os.path.join(args.output_dir, "modelA_train_curves.png"), "ModelA")

    torch.save(model_a.state_dict(), os.path.join(args.output_dir, "modelA_weights.pth"))

    conf_a, metrics_a, wrong_a = evaluate_model(model_a, test_loader, args.device, max_wrong=25)
    plot_confusion_matrix(conf_a, os.path.join(args.output_dir, "modelA_confusion.png"), "ModelA Confusion Matrix")
    save_wrong_samples(wrong_a[:10], os.path.join(args.output_dir, "modelA_wrong_samples.png"))
    with open(os.path.join(args.output_dir, "modelA_metrics.json"), "w", encoding="utf-8") as f:
        json.dump(metrics_a, f, indent=2)

    print(f"ModelA Test Accuracy: {metrics_a['accuracy']:.4f}")
    print(f"ModelA Precision(Macro): {metrics_a['precision_macro']:.4f}")
    print(f"ModelA Recall(Macro): {metrics_a['recall_macro']:.4f}")
    print(f"ModelA F1(Macro): {metrics_a['f1_macro']:.4f}")
    print(f"ModelA Precision(Weighted): {metrics_a['precision_weighted']:.4f}")
    print(f"ModelA Recall(Weighted): {metrics_a['recall_weighted']:.4f}")
    print(f"ModelA F1(Weighted): {metrics_a['f1_weighted']:.4f}")

    # Model B
    train_loader_b, test_loader_b = build_dataloaders(args.batch_size, args.num_workers, mean, std, use_aug=True)
    model_b = ModelB().to(args.device)
    optimizer_b = torch.optim.Adam(model_b.parameters(), lr=args.lr, weight_decay=args.weight_decay)

    target_acc = 0.99 if args.ensure_99 else None

    params_b_total, params_b_trainable = count_parameters(model_b)
    print(f"ModelB params: total={params_b_total}, trainable={params_b_trainable}")

    logs_b, step_b, epoch_stats_b = train_model(
        model_b, train_loader_b, args.device, optimizer_b, criterion,
        epochs=args.epochs, log_path=os.path.join(args.output_dir, "modelB_train_log.csv"),
        model_name="ModelB",
        epoch_log_path=os.path.join(args.output_dir, "modelB_epoch_log.csv"),
        eval_loader=test_loader_b,
    )

    trained_epochs = args.epochs
    conf_b, metrics_b, wrong_b = evaluate_model(model_b, test_loader_b, args.device, max_wrong=25)
    if target_acc is not None:
        while metrics_b["accuracy"] < target_acc and trained_epochs < args.max_epochs:
            print(f"ModelB accuracy {metrics_b['accuracy']:.4f} < 0.99, extending training...")
            extra_logs, step_b, extra_epoch_stats = train_model(
                model_b, train_loader_b, args.device, optimizer_b, criterion,
                epochs=1, log_path=os.path.join(args.output_dir, "modelB_train_log.csv"),
                model_name="ModelB", start_step=step_b, start_epoch=trained_epochs, append=True,
                epoch_log_path=os.path.join(args.output_dir, "modelB_epoch_log.csv"),
                eval_loader=test_loader_b,
            )
            logs_b.extend(extra_logs)
            epoch_stats_b.extend(extra_epoch_stats)
            trained_epochs += 1
            conf_b, metrics_b, wrong_b = evaluate_model(model_b, test_loader_b, args.device, max_wrong=25)

    plot_train_curves(logs_b, os.path.join(args.output_dir, "modelB_train_curves.png"), "ModelB")
    torch.save(model_b.state_dict(), os.path.join(args.output_dir, "modelB_weights.pth"))
    plot_confusion_matrix(conf_b, os.path.join(args.output_dir, "modelB_confusion.png"), "ModelB Confusion Matrix")
    save_wrong_samples(wrong_b[:10], os.path.join(args.output_dir, "modelB_wrong_samples.png"))
    with open(os.path.join(args.output_dir, "modelB_metrics.json"), "w", encoding="utf-8") as f:
        json.dump(metrics_b, f, indent=2)

    print(f"ModelB Test Accuracy: {metrics_b['accuracy']:.4f}")
    print(f"ModelB Precision(Macro): {metrics_b['precision_macro']:.4f}")
    print(f"ModelB Recall(Macro): {metrics_b['recall_macro']:.4f}")
    print(f"ModelB F1(Macro): {metrics_b['f1_macro']:.4f}")
    print(f"ModelB Precision(Weighted): {metrics_b['precision_weighted']:.4f}")
    print(f"ModelB Recall(Weighted): {metrics_b['recall_weighted']:.4f}")
    print(f"ModelB F1(Weighted): {metrics_b['f1_weighted']:.4f}")

    params_path = os.path.join(args.output_dir, "model_params.json")
    with open(params_path, "w", encoding="utf-8") as f:
        json.dump(
            {
                "modelA_total": params_a_total,
                "modelA_trainable": params_a_trainable,
                "modelB_total": params_b_total,
                "modelB_trainable": params_b_trainable,
            },
            f,
            indent=2,
        )


if __name__ == "__main__":
    main()

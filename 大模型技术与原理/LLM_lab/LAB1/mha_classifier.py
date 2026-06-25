#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
基于自定义 Multi-Head Attention 的疾病领域分类模型

"""

import argparse
import json
import math
import random
from typing import Dict, List, Tuple

import torch
from torch import nn
from torch.utils.data import Dataset, DataLoader, random_split

# Matplotlib 仅用于可视化，不存在时不影响训练
try:
    import matplotlib.pyplot as plt
    import matplotlib
    # 全局设置中文字体，避免方框
    matplotlib.rcParams["font.sans-serif"] = [
        "SimHei", "Microsoft YaHei", "SimSun", "DengXian",
        "Arial Unicode MS", "Noto Sans CJK SC", "WenQuanYi Micro Hei",
    ]
    matplotlib.rcParams["axes.unicode_minus"] = False
    HAS_PLT = True
except Exception:
    HAS_PLT = False

DEVICE = torch.device("cuda" if torch.cuda.is_available() else "cpu")


# ------------------------------------------------------------
# 工具函数：随机种子 & 简易指标
# ------------------------------------------------------------
def set_seed(seed: int = 2024) -> None:
    """固定随机种子，保证实验可复现。"""
    random.seed(seed)
    torch.manual_seed(seed)
    torch.cuda.manual_seed_all(seed)
    torch.backends.cudnn.deterministic = True
    torch.backends.cudnn.benchmark = False


def accuracy(pred: torch.Tensor, gold: torch.Tensor) -> float:
    """计算分类准确率。"""
    return (pred == gold).float().mean().item()


def macro_f1(pred: torch.Tensor, gold: torch.Tensor, num_classes: int) -> float:
    """
    纯 PyTorch 实现的 macro-F1，避免额外依赖 sklearn。
    注意：加上 1e-8 防止除零。
    """
    f1_list = []
    for c in range(num_classes):
        tp = ((pred == c) & (gold == c)).sum().float()
        fp = ((pred == c) & (gold != c)).sum().float()
        fn = ((pred != c) & (gold == c)).sum().float()
        precision = tp / (tp + fp + 1e-8)
        recall = tp / (tp + fn + 1e-8)
        f1 = 2 * precision * recall / (precision + recall + 1e-8)
        f1_list.append(f1)
    return torch.stack(f1_list).mean().item()


# ------------------------------------------------------------
# 词表与数据集
# ------------------------------------------------------------
class Vocabulary:
    """
    简易词表：记录 token->id 与 id->token。
    由于数据本身是结构化症状词，无需复杂分词。
    """
    def __init__(self):
        self.token2id: Dict[str, int] = {"[PAD]": 0, "[UNK]": 1}
        self.id2token: List[str] = ["[PAD]", "[UNK]"]

    def add_token(self, token: str) -> None:
        if token not in self.token2id:
            self.token2id[token] = len(self.id2token)
            self.id2token.append(token)

    def encode(self, token: str) -> int:
        return self.token2id.get(token, 1)

    def decode(self, idx: int) -> str:
        if 0 <= idx < len(self.id2token):
            return self.id2token[idx]
        return "[UNK]"

    @property
    def size(self) -> int:
        return len(self.id2token)


def record_to_tokens(record: Dict, include_neg: bool = True) -> List[str]:
    """
    将一条病历转为“单词序列”。
    - 对值为 1 的症状直接保留；
    - include_neg=True 时，对值为 0 的症状加前缀“无”保留，可帮助模型感知缺失信息。
    """
    tokens: List[str] = []
    for k, v in record["exp_sxs"].items():
        tokens.append(k if v == "1" else f"无{k}" if include_neg else None)
    for k, v in record["imp_sxs"].items():
        tokens.append(k if v == "1" else f"无{k}" if include_neg else None)
    return [t for t in tokens if t is not None]


class SymptomDataset(Dataset):
    """
    将 JSON 数据封装为 PyTorch Dataset，并完成：
    - 构建词表
    - 将样本转为定长 token id 序列与 mask
    """
    def __init__(
        self,
        path: str,
        vocab: Vocabulary = None,
        label2id: Dict[str, int] = None,
        max_len: int = 128,
        include_neg: bool = True,
    ):
        super().__init__()
        with open(path, "r", encoding="utf8") as f:
            self.raw = json.load(f)
        self.max_len = max_len
        self.include_neg = include_neg

        # 标签映射：若未传入则按字典序创建
        if label2id is None:
            labels = sorted({r["label"] for r in self.raw})
            self.label2id = {lb: i for i, lb in enumerate(labels)}
        else:
            self.label2id = label2id
        self.id2label = {v: k for k, v in self.label2id.items()}

        # 词表：若未传入则用全量数据构建
        self.vocab = vocab or Vocabulary()
        if vocab is None:
            for r in self.raw:
                for t in record_to_tokens(r, include_neg=self.include_neg):
                    self.vocab.add_token(t)

        # 预编码样本
        self.samples: List[Tuple[torch.Tensor, torch.Tensor, torch.Tensor]] = []
        for r in self.raw:
            ids, mask = self.encode_tokens(record_to_tokens(r, include_neg=self.include_neg))
            label_id = self.label2id.get(r["label"], -1)
            self.samples.append((ids, mask, torch.tensor(label_id, dtype=torch.long)))

    def encode_tokens(self, tokens: List[str]) -> Tuple[torch.Tensor, torch.Tensor]:
        """将 token 列表转为定长 id 序列与 mask。"""
        ids = [self.vocab.encode(t) for t in tokens][: self.max_len]
        mask = [1] * len(ids)
        pad_len = self.max_len - len(ids)
        if pad_len > 0:
            ids += [0] * pad_len
            mask += [0] * pad_len
        return torch.tensor(ids, dtype=torch.long), torch.tensor(mask, dtype=torch.float32)

    def __len__(self):
        return len(self.samples)

    def __getitem__(self, idx: int):
        return self.samples[idx]


# ------------------------------------------------------------
# 模型组件：位置编码、注意力、FFN、Encoder
# ------------------------------------------------------------
class PositionalEncoding(nn.Module):
    """标准正弦位置编码。"""
    def __init__(self, d_model: int, dropout: float = 0.1, max_len: int = 512):
        super().__init__()
        self.dropout = nn.Dropout(dropout)

        pe = torch.zeros(max_len, d_model)
        position = torch.arange(0, max_len, dtype=torch.float).unsqueeze(1)
        div_term = torch.exp(torch.arange(0, d_model, 2).float() * (-math.log(10000.0) / d_model))
        pe[:, 0::2] = torch.sin(position * div_term)
        pe[:, 1::2] = torch.cos(position * div_term)
        pe = pe.unsqueeze(0)  # [1, max_len, d_model]
        self.register_buffer("pe", pe)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        x = x + self.pe[:, : x.size(1), :]
        return self.dropout(x)


class ScaledDotProductAttention(nn.Module):
    """
    手写缩放点积注意力：
    - 可通过 use_scale 控制是否除以 sqrt(d_k)
    - mask 为 0 的位置填充 -1e9，确保 Softmax 后权重接近 0
    """
    def __init__(self, dropout: float = 0.1, use_scale: bool = True):
        super().__init__()
        self.dropout = nn.Dropout(dropout)
        self.use_scale = use_scale

    def forward(
        self,
        Q: torch.Tensor,
        K: torch.Tensor,
        V: torch.Tensor,
        mask: torch.Tensor = None,
    ) -> Tuple[torch.Tensor, torch.Tensor]:
        """
        Q, K, V: [batch, head, seq_len, d_k]
        mask:    [batch, 1, 1, seq_len]，1 表示有效，0 表示 PAD
        """
        d_k = Q.size(-1)
        scores = torch.matmul(Q, K.transpose(-2, -1))  # [B, h, L, L]
        if self.use_scale:
            scores = scores / math.sqrt(d_k)
        if mask is not None:
            scores = scores.masked_fill(mask == 0, -1e9)
        attn = torch.softmax(scores, dim=-1)
        attn = self.dropout(attn)
        output = torch.matmul(attn, V)  # [B, h, L, d_k]
        return output, attn


class MultiHeadAttention(nn.Module):
    """
    多头注意力，完全调用上面的 ScaledDotProductAttention。
    """
    def __init__(self, d_model: int, num_heads: int, dropout: float = 0.1, use_scale: bool = True):
        super().__init__()
        assert d_model % num_heads == 0, "d_model 必须能被 num_heads 整除"
        self.num_heads = num_heads
        self.d_k = d_model // num_heads
        # debug_shapes 用于维度追踪绘图
        self.debug_shapes: Dict[str, Tuple[int, ...]] = {}

        self.W_Q = nn.Linear(d_model, d_model)
        self.W_K = nn.Linear(d_model, d_model)
        self.W_V = nn.Linear(d_model, d_model)
        self.fc = nn.Linear(d_model, d_model)
        self.attention = ScaledDotProductAttention(dropout=dropout, use_scale=use_scale)
        self.dropout = nn.Dropout(dropout)

    def forward(
        self,
        query: torch.Tensor,
        key: torch.Tensor,
        value: torch.Tensor,
        mask: torch.Tensor = None,
        return_attn: bool = False,
    ) -> Tuple[torch.Tensor, torch.Tensor]:
        B, L, _ = query.size()

        # 线性变换后分头
        Q = self.W_Q(query).view(B, L, self.num_heads, self.d_k).transpose(1, 2)
        K = self.W_K(key).view(B, L, self.num_heads, self.d_k).transpose(1, 2)
        V = self.W_V(value).view(B, L, self.num_heads, self.d_k).transpose(1, 2)

        # 维度追踪：记录进入 MHA 前 / 分头后形状
        self.debug_shapes["before_attn"] = tuple(query.size())          # [B, L, d_model]
        self.debug_shapes["after_split"] = tuple(Q.size())              # [B, num_heads, L, d_k]

        # 缩放点积注意力
        context, attn = self.attention(Q, K, V, mask)

        # 拼接各头
        context = context.transpose(1, 2).contiguous().view(B, L, self.num_heads * self.d_k)
        self.debug_shapes["after_concat"] = tuple(context.size())       # [B, L, d_model]
        out = self.fc(context)
        out = self.dropout(out)
        return (out, attn) if return_attn else (out, None)


class PositionwiseFFN(nn.Module):
    """前馈网络：Linear -> ReLU -> Dropout -> Linear。"""
    def __init__(self, d_model: int, d_ff: int, dropout: float = 0.1):
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(d_model, d_ff),
            nn.ReLU(),
            nn.Dropout(dropout),
            nn.Linear(d_ff, d_model),
        )

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return self.net(x)


class TransformerEncoderLayer(nn.Module):
    """
    单层 Encoder：MHA + 残差 + LayerNorm + FFN。
    return_attn=True 时会返回注意力矩阵用于可视化。
    """
    def __init__(
        self,
        d_model: int = 128,
        num_heads: int = 4,
        d_ff: int = 256,
        dropout: float = 0.1,
        use_scale: bool = True,
    ):
        super().__init__()
        self.mha = MultiHeadAttention(d_model, num_heads, dropout=dropout, use_scale=use_scale)
        self.norm1 = nn.LayerNorm(d_model)
        self.norm2 = nn.LayerNorm(d_model)
        self.ffn = PositionwiseFFN(d_model, d_ff, dropout=dropout)
        self.dropout = nn.Dropout(dropout)

    def forward(self, x: torch.Tensor, mask: torch.Tensor = None, return_attn: bool = False):
        # Self-Attention
        attn_out, attn = self.mha(x, x, x, mask, return_attn=return_attn)
        x = self.norm1(x + self.dropout(attn_out))
        # FFN
        ffn_out = self.ffn(x)
        x = self.norm2(x + self.dropout(ffn_out))
        return (x, attn) if return_attn else (x, None)


class TransformerClassifier(nn.Module):
    """
    编码器 + 池化 + 全连接分类头。
    池化方式：对非 PAD 位置做平均（mask 加权）。
    """
    def __init__(
        self,
        vocab_size: int,
        num_classes: int,
        d_model: int = 128,
        num_heads: int = 4,
        d_ff: int = 256,
        dropout: float = 0.1,
        max_len: int = 128,
        use_scale: bool = True,
    ):
        super().__init__()
        self.embedding = nn.Embedding(vocab_size, d_model, padding_idx=0)
        self.pos_enc = PositionalEncoding(d_model, dropout=dropout, max_len=max_len)
        self.encoder = TransformerEncoderLayer(
            d_model=d_model,
            num_heads=num_heads,
            d_ff=d_ff,
            dropout=dropout,
            use_scale=use_scale,
        )
        self.classifier = nn.Linear(d_model, num_classes)

    def forward(self, ids: torch.Tensor, mask: torch.Tensor, return_attn: bool = False):
        """
        ids:  [B, L] token id 序列
        mask: [B, L] 1 表示真实 token，0 表示 PAD
        """
        x = self.embedding(ids)
        x = self.pos_enc(x)

        # 注意力 mask 维度需为 [B, 1, 1, L]
        attn_mask = mask.unsqueeze(1).unsqueeze(2)
        enc_out, attn = self.encoder(x, mask=attn_mask, return_attn=return_attn)

        # Masked mean pooling
        mask_expanded = mask.unsqueeze(-1)  # [B, L, 1]
        sum_pool = (enc_out * mask_expanded).sum(dim=1)
        denom = mask_expanded.sum(dim=1).clamp(min=1e-6)
        pooled = sum_pool / denom

        logits = self.classifier(pooled)
        return (logits, attn, enc_out) if return_attn else logits


# ------------------------------------------------------------
# 训练与评估
# ------------------------------------------------------------
def train_one_epoch(
    model: nn.Module,
    loader: DataLoader,
    optimizer: torch.optim.Optimizer,
    criterion: nn.Module,
    num_classes: int,
) -> Dict[str, float]:
    model.train()
    total_loss, total_acc, total_f1, n_batch = 0.0, 0.0, 0.0, 0
    for ids, mask, labels in loader:
        ids, mask, labels = ids.to(DEVICE), mask.to(DEVICE), labels.to(DEVICE)
        optimizer.zero_grad()
        logits = model(ids, mask)
        loss = criterion(logits, labels)
        loss.backward()
        optimizer.step()

        preds = torch.argmax(logits, dim=-1)
        total_loss += loss.item()
        total_acc += accuracy(preds, labels)
        total_f1 += macro_f1(preds, labels, num_classes)
        n_batch += 1

    return {
        "loss": total_loss / n_batch,
        "acc": total_acc / n_batch,
        "f1": total_f1 / n_batch,
    }


@torch.no_grad()
def evaluate(
    model: nn.Module,
    loader: DataLoader,
    criterion: nn.Module,
    num_classes: int,
) -> Dict[str, float]:
    model.eval()
    total_loss, total_acc, total_f1, n_batch = 0.0, 0.0, 0.0, 0
    for ids, mask, labels in loader:
        ids, mask, labels = ids.to(DEVICE), mask.to(DEVICE), labels.to(DEVICE)
        logits = model(ids, mask)
        loss = criterion(logits, labels)
        preds = torch.argmax(logits, dim=-1)
        total_loss += loss.item()
        total_acc += accuracy(preds, labels)
        total_f1 += macro_f1(preds, labels, num_classes)
        n_batch += 1

    return {
        "loss": total_loss / n_batch,
        "acc": total_acc / n_batch,
        "f1": total_f1 / n_batch,
    }


# ------------------------------------------------------------
# 可视化注意力：随机抽样一条样本
# ------------------------------------------------------------
@torch.no_grad()
def visualize_attention(
    model: TransformerClassifier,
    dataset: SymptomDataset,
    head: int = 0,
    save_path: str = "attention.png",
) -> None:
    """
    简易可视化：绘制第一层指定头的注意力热力图。
    - 只展示非 PAD 部分，便于阅读。
    """
    if not HAS_PLT:
        print("未安装 matplotlib，无法绘制注意力图。可执行 `pip install matplotlib` 后再试。")
        return

    idx = random.randint(0, len(dataset) - 1)
    ids, mask, label = dataset[idx]
    valid_len = int(mask.sum().item())
    ids = ids.unsqueeze(0).to(DEVICE)
    mask_batch = mask.unsqueeze(0).to(DEVICE)

    logits, attn, _ = model(ids, mask_batch, return_attn=True)
    attn = attn[:, head, :valid_len, :valid_len].squeeze(0).cpu()

    tokens = [dataset.vocab.decode(i) for i in ids[0][:valid_len].tolist()]
    plt.figure(figsize=(8, 6))
    plt.imshow(attn, cmap="YlGnBu")
    plt.xticks(range(valid_len), tokens, rotation=60, fontsize=8)
    plt.yticks(range(valid_len), tokens, fontsize=8)
    title = f"Sample {idx} | Label: {dataset.id2label[int(label)]} | Head {head}"
    plt.title(title)
    plt.colorbar()
    plt.tight_layout()
    plt.savefig(save_path, dpi=300)
    print(f"注意力图已保存到 {save_path}")


# ------------------------------------------------------------
# 维度追踪图绘制
# ------------------------------------------------------------
def plot_tensor_shapes(shapes: Dict[str, Tuple[int, ...]], save_path: str = "tensor_shapes.png") -> None:
    """
    简单绘制三个节点的维度流向图：
    before_attn -> after_split -> after_concat
    """
    if not HAS_PLT:
        print("未安装 matplotlib，无法绘制维度变化图。可执行 `pip install matplotlib` 后再试。")
        return

    labels = [
        ("Before Attention", shapes.get("before_attn", ())),
        ("After Split", shapes.get("after_split", ())),
        ("After Concat", shapes.get("after_concat", ())),
    ]

    fig, ax = plt.subplots(figsize=(8, 3))
    ax.axis("off")

    x_positions = [0.1, 0.5, 0.9]
    y = 0.5
    box_props = dict(boxstyle="round,pad=0.4", facecolor="#e0f3ff", edgecolor="#1f78b4")

    for (title, shape), x in zip(labels, x_positions):
        text = f"{title}\n{shape}"
        ax.text(x, y, text, ha="center", va="center", bbox=box_props, fontsize=10)

    # 画箭头
    ax.annotate("", xy=(x_positions[1]-0.1, y), xytext=(x_positions[0]+0.1, y),
                arrowprops=dict(arrowstyle="->", lw=2, color="#1f78b4"))
    ax.annotate("", xy=(x_positions[2]-0.1, y), xytext=(x_positions[1]+0.1, y),
                arrowprops=dict(arrowstyle="->", lw=2, color="#1f78b4"))

    plt.tight_layout()
    plt.savefig(save_path, dpi=300)
    print(f"维度变化图已保存到 {save_path}")


def plot_loss_curves(history: Dict[str, List[float]], save_path: str) -> None:
    """
    绘制训练/验证 Loss 与 F1 曲线，便于实验报告直接使用。
    """
    if not HAS_PLT:
        print("未安装 matplotlib，无法绘制 Loss 曲线。可执行 `pip install matplotlib` 后再试。")
        return

    epochs = range(1, len(history["train_loss"]) + 1)
    plt.figure(figsize=(8, 4))
    plt.subplot(1, 2, 1)
    plt.plot(epochs, history["train_loss"], label="train_loss")
    plt.plot(epochs, history["val_loss"], label="val_loss")
    plt.xlabel("epoch")
    plt.ylabel("loss")
    plt.title("Loss")
    plt.legend()

    plt.subplot(1, 2, 2)
    plt.plot(epochs, history["train_f1"], label="train_f1")
    plt.plot(epochs, history["val_f1"], label="val_f1")
    plt.xlabel("epoch")
    plt.ylabel("F1")
    plt.title("F1")
    plt.legend()

    plt.tight_layout()
    plt.savefig(save_path, dpi=300)
    print(f"Loss/F1 曲线已保存到 {save_path}")


# ------------------------------------------------------------
# 主训练流程
# ------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(description="自定义 MHA 疾病分类")
    parser.add_argument("--train", type=str, default="train_set.json", help="训练集路径")
    parser.add_argument("--test", type=str, default="test_set.json", help="测试/验证集路径")
    parser.add_argument("--max_len", type=int, default=128, help="序列最大长度（截断或填充）")
    parser.add_argument("--batch_size", type=int, default=32, help="批大小")
    parser.add_argument("--epochs", type=int, default=10, help="训练轮次")
    parser.add_argument("--lr", type=float, default=2e-3, help="学习率")
    parser.add_argument("--d_model", type=int, default=128, help="模型隐藏维度")
    parser.add_argument("--num_heads", type=int, default=4, help="注意力头数")
    parser.add_argument("--d_ff", type=int, default=256, help="FFN 隐藏维度")
    parser.add_argument("--dropout", type=float, default=0.1, help="Dropout 比例")
    parser.add_argument("--include_neg", action="store_true", help="是否保留值为 0 的症状（前缀“无”）")
    parser.add_argument("--no_scale", action="store_true", help="不使用 1/sqrt(d_k) 缩放因子，用于对比实验")
    parser.add_argument("--visualize", action="store_true", help="训练后绘制一条样本的注意力图")
    parser.add_argument("--plot_shapes", action="store_true", help="保存一次前向中的维度变化示意图")
    parser.add_argument("--seed", type=int, default=2024, help="随机种子")
    args = parser.parse_args()

    set_seed(args.seed)

    # 构建数据集与词表
    full_train = SymptomDataset(
        args.train,
        vocab=None,
        label2id=None,
        max_len=args.max_len,
        include_neg=args.include_neg,
    )
    # 简单划分训练/验证（9:1）
    val_size = max(1, int(0.1 * len(full_train)))
    train_size = len(full_train) - val_size
    train_ds, val_ds = random_split(full_train, [train_size, val_size])

    test_ds = SymptomDataset(
        args.test,
        vocab=full_train.vocab,
        label2id=full_train.label2id,
        max_len=args.max_len,
        include_neg=args.include_neg,
    )

    train_loader = DataLoader(train_ds, batch_size=args.batch_size, shuffle=True)
    val_loader = DataLoader(val_ds, batch_size=args.batch_size)
    test_loader = DataLoader(test_ds, batch_size=args.batch_size)

    model = TransformerClassifier(
        vocab_size=full_train.vocab.size,
        num_classes=len(full_train.label2id),
        d_model=args.d_model,
        num_heads=args.num_heads,
        d_ff=args.d_ff,
        dropout=args.dropout,
        max_len=args.max_len,
        use_scale=not args.no_scale,
    ).to(DEVICE)

    optimizer = torch.optim.Adam(model.parameters(), lr=args.lr)
    criterion = nn.CrossEntropyLoss()

    print(f"设备: {DEVICE} | 词表大小: {full_train.vocab.size} | 类别数: {len(full_train.label2id)}")
    print(f"使用缩放因子: {not args.no_scale}")

    best_val_f1, best_state = 0.0, None
    history = {"train_loss": [], "val_loss": [], "train_f1": [], "val_f1": []}

    for epoch in range(1, args.epochs + 1):
        train_metrics = train_one_epoch(model, train_loader, optimizer, criterion, len(full_train.label2id))
        val_metrics = evaluate(model, val_loader, criterion, len(full_train.label2id))

        history["train_loss"].append(train_metrics["loss"])
        history["val_loss"].append(val_metrics["loss"])
        history["train_f1"].append(train_metrics["f1"])
        history["val_f1"].append(val_metrics["f1"])

        print(
            f"Epoch {epoch:02d} | "
            f"Train Loss {train_metrics['loss']:.4f} Acc {train_metrics['acc']:.4f} F1 {train_metrics['f1']:.4f} || "
            f"Val Loss {val_metrics['loss']:.4f} Acc {val_metrics['acc']:.4f} F1 {val_metrics['f1']:.4f}"
        )

        # 记录最佳模型（按验证 F1）
        if val_metrics["f1"] > best_val_f1:
            best_val_f1 = val_metrics["f1"]
            best_state = model.state_dict()

    # 测试集评估
    if best_state:
        model.load_state_dict(best_state)
    test_metrics = evaluate(model, test_loader, criterion, len(full_train.label2id))
    print(
        f"[TEST] Loss {test_metrics['loss']:.4f} Acc {test_metrics['acc']:.4f} F1 {test_metrics['f1']:.4f}"
    )

    # 记录 Loss 曲线，便于对比缩放/不缩放
    loss_json_name = "loss_history_no_scale.json" if args.no_scale else "loss_history.json"
    with open(loss_json_name, "w", encoding="utf8") as f:
        json.dump(history, f, ensure_ascii=False, indent=2)
    print(f"训练/验证 Loss 与 F1 曲线数据已写入 {loss_json_name}，可用于报告绘图对比。")

    # 绘制并保存 Loss/F1 曲线图（文件名区分是否使用缩放）
    loss_png_name = "loss_curve_no_scale.png" if args.no_scale else "loss_curve.png"
    plot_loss_curves(history, save_path=loss_png_name)

    # 可选：绘制注意力
    if args.visualize:
        visualize_attention(model, test_ds, head=0, save_path="attention.png")

    # 可选：维度追踪图（取一小批数据正向传播并记录形状）
    if args.plot_shapes:
        model.eval()
        with torch.no_grad():
            sample_ids, sample_mask, _ = next(iter(train_loader))
            sample_ids = sample_ids.to(DEVICE)
            sample_mask = sample_mask.to(DEVICE)
            _ = model(sample_ids, sample_mask)  # 正向传播以填充 debug_shapes
            shapes = model.encoder.mha.debug_shapes
            print("维度追踪：", shapes)
            plot_tensor_shapes(shapes, save_path="tensor_shapes.png")


if __name__ == "__main__":
    main()

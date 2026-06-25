import json
import os
from collections import Counter
from typing import Dict, List, Tuple

import torch
import torch.nn as nn
from torch.utils.data import DataLoader, Dataset


def load_text(file_path: str) -> List[str]:
    """Read raw text and return a simple character-level token list."""
    with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
        text = f.read()
    # Strip whitespace-only tokens to keep the sequence dense.
    tokens = [ch for ch in text if not ch.isspace()]
    return tokens


def build_vocab(tokens: List[str], max_vocab_size: int = 30000) -> Tuple[Dict[str, int], Dict[int, str]]:
    """Build a vocab capped at max_vocab_size with <pad>/<unk> specials."""
    counter = Counter(tokens)
    specials = ["<pad>", "<unk>"]
    most_common = counter.most_common(max_vocab_size - len(specials))

    word2id: Dict[str, int] = {tok: idx for idx, tok in enumerate(specials)}
    for tok, _ in most_common:
        word2id[tok] = len(word2id)

    id2word: Dict[int, str] = {idx: tok for tok, idx in word2id.items()}
    return word2id, id2word


def encode_tokens(tokens: List[str], word2id: Dict[str, int]) -> List[int]:
    unk_id = word2id["<unk>"]
    return [word2id.get(tok, unk_id) for tok in tokens]


class TextDataset(Dataset):
    """Sliding-window LM dataset: input tokens predict the next token."""

    def __init__(self, ids: List[int], seq_len: int):
        if len(ids) < seq_len + 1:
            raise ValueError("Not enough tokens to create sequences.")
        self.ids = ids
        self.seq_len = seq_len

    def __len__(self) -> int:
        return len(self.ids) - self.seq_len

    def __getitem__(self, idx: int):
        x = torch.tensor(self.ids[idx : idx + self.seq_len], dtype=torch.long)
        y = torch.tensor(self.ids[idx + 1 : idx + self.seq_len + 1], dtype=torch.long)
        return x, y


class LSTMLanguageModel(nn.Module):
    def __init__(self, vocab_size: int, embed_dim: int = 64, hidden_dim: int = 128):
        super().__init__()
        self.embedding = nn.Embedding(vocab_size, embed_dim, padding_idx=0)
        self.lstm = nn.LSTM(embed_dim, hidden_dim, num_layers=1, batch_first=True)
        self.output = nn.Linear(hidden_dim, vocab_size)

    def forward(self, x, hidden=None):
        embed = self.embedding(x)
        out, hidden = self.lstm(embed, hidden)
        logits = self.output(out)
        return logits, hidden


def save_vocab(word2id: Dict[str, int], id2word: Dict[int, str], save_dir: str):
    os.makedirs(save_dir, exist_ok=True)
    with open(os.path.join(save_dir, "word2id_lstm.json"), "w", encoding="utf-8") as f:
        json.dump(word2id, f, ensure_ascii=False, indent=2)
    with open(os.path.join(save_dir, "id2word_lstm.json"), "w", encoding="utf-8") as f:
        json.dump(id2word, f, ensure_ascii=False, indent=2)


def train(
    data_path: str = "./data/text.txt",
    ckpt_dir: str = "./ckpt",
    seq_len: int = 31,
    embed_dim: int = 64,
    hidden_dim: int = 256,
    max_vocab_size: int = 30000,
    batch_size: int = 256  ,
    epochs: int = 15,
    lr: float = 3e-4,
):
    torch.manual_seed(42)
    base_dir = os.path.dirname(os.path.abspath(__file__))
    data_path = os.path.join(base_dir, data_path)
    ckpt_dir = os.path.join(base_dir, ckpt_dir)
    os.makedirs(ckpt_dir, exist_ok=True)

    print(f"Loading text from {data_path} ...")
    tokens = load_text(data_path)
    word2id, id2word = build_vocab(tokens, max_vocab_size=max_vocab_size)
    ids = encode_tokens(tokens, word2id)
    vocab_size = len(word2id)

    dataset = TextDataset(ids, seq_len=seq_len)
    dataloader = DataLoader(dataset, batch_size=batch_size, shuffle=True, drop_last=True, num_workers=0)

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    model = LSTMLanguageModel(vocab_size=vocab_size, embed_dim=embed_dim, hidden_dim=hidden_dim).to(device)
    criterion = nn.CrossEntropyLoss(ignore_index=0)
    optimizer = torch.optim.AdamW(model.parameters(), lr=lr, weight_decay=1e-4)

    best_loss = float("inf")
    for epoch in range(1, epochs + 1):
        model.train()
        total_loss = 0.0
        for batch_x, batch_y in dataloader:
            batch_x = batch_x.to(device)
            batch_y = batch_y.to(device)

            optimizer.zero_grad()
            logits, _ = model(batch_x)
            loss = criterion(logits.view(-1, vocab_size), batch_y.view(-1))
            loss.backward()
            torch.nn.utils.clip_grad_norm_(model.parameters(), max_norm=1.0)
            optimizer.step()
            total_loss += loss.item()

        avg_loss = total_loss / len(dataloader)
        print(f"Epoch {epoch}/{epochs} - loss: {avg_loss:.4f}")

        if avg_loss < best_loss:
            best_loss = avg_loss
            save_path = os.path.join(ckpt_dir, "lstm_lm.pt")
            torch.save(
                {
                    "model_state": model.state_dict(),
                    "word2id": word2id,
                    "id2word": id2word,
                    "config": {
                        "seq_len": seq_len,
                        "embed_dim": embed_dim,
                        "hidden_dim": hidden_dim,
                        "max_vocab_size": max_vocab_size,
                    },
                    "loss": avg_loss,
                },
                save_path,
            )
            print(f"  Saved checkpoint to {save_path}")

    save_vocab(word2id, id2word, save_dir=os.path.join(base_dir, "data"))
    print(f"Training complete. Best loss: {best_loss:.4f}")


if __name__ == "__main__":
    train()

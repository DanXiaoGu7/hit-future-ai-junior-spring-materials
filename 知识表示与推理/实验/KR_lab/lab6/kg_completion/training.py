from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Sequence, Tuple
import random

import torch
from torch import nn
from torch.utils.data import DataLoader, Dataset

from .sampling import NegativeSampler

Triple = Tuple[int, int, int]


class TripleDataset(Dataset):
    def __init__(self, triples: Sequence[Triple]) -> None:
        self.triples = torch.tensor(list(triples), dtype=torch.long)

    def __len__(self) -> int:
        return self.triples.size(0)

    def __getitem__(self, idx: int) -> torch.Tensor:
        return self.triples[idx]


@dataclass
class TrainConfig:
    epochs: int
    batch_size: int
    lr: float
    margin: float
    reg_weight: float
    negative_size: int
    sampling: str
    max_grad_norm: float
    num_workers: int
    log_every: int
    seed: int
    device: torch.device


def train_model(
    model: nn.Module,
    train_triples: Sequence[Triple],
    all_true_triples: set[Triple],
    ent_num: int,
    config: TrainConfig,
) -> None:
    dataset = TripleDataset(train_triples)
    generator = torch.Generator()
    generator.manual_seed(config.seed)
    loader = DataLoader(
        dataset,
        batch_size=config.batch_size,
        shuffle=True,
        num_workers=config.num_workers,
        generator=generator,
    )
    sampler = NegativeSampler(
        ent_num=ent_num,
        train_triples=train_triples,
        all_true_triples=all_true_triples,
        mode=config.sampling,
        negative_size=config.negative_size,
    )
    optimizer = torch.optim.Adam(model.parameters(), lr=config.lr)
    criterion = nn.MarginRankingLoss(margin=config.margin)

    model.to(config.device)
    for epoch in range(1, config.epochs + 1):
        model.train()
        total_loss = 0.0
        seen = 0

        for positive in loader:
            positive = positive.to(config.device)
            negative = sampler.sample(positive)

            if config.negative_size > 1:
                positive_for_loss = positive.repeat_interleave(config.negative_size, dim=0)
            else:
                positive_for_loss = positive

            pos_score = score_triples(model, positive_for_loss)
            neg_score = score_triples(model, negative)
            target = -torch.ones_like(pos_score)
            loss = criterion(pos_score, neg_score, target)

            if config.reg_weight > 0:
                loss = loss + config.reg_weight * model.regularization(positive_for_loss)

            optimizer.zero_grad()
            loss.backward()
            if config.max_grad_norm > 0:
                nn.utils.clip_grad_norm_(model.parameters(), config.max_grad_norm)
            optimizer.step()
            if hasattr(model, "post_step"):
                model.post_step()

            batch_size = positive.size(0)
            total_loss += loss.item() * batch_size
            seen += batch_size

        if config.log_every > 0 and (epoch == 1 or epoch % config.log_every == 0):
            print(f"epoch={epoch:04d} loss={total_loss / max(seen, 1):.6f}")


def score_triples(model: nn.Module, triples: torch.Tensor) -> torch.Tensor:
    return model.score(triples[:, 0], triples[:, 2], triples[:, 1])


def save_checkpoint(model: nn.Module, path: str | Path) -> None:
    checkpoint_path = Path(path)
    checkpoint_path.parent.mkdir(parents=True, exist_ok=True)
    torch.save(model.state_dict(), checkpoint_path)


def load_checkpoint(model: nn.Module, path: str | Path, device: torch.device) -> None:
    state = torch.load(Path(path), map_location=device)
    model.load_state_dict(state)


def set_seed(seed: int) -> None:
    random.seed(seed)
    torch.manual_seed(seed)
    if torch.cuda.is_available():
        torch.cuda.manual_seed_all(seed)

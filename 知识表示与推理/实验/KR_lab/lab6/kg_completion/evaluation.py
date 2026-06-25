from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass
from typing import Dict, Iterable, List, Sequence, Tuple

import torch
from torch import nn

from .data import batch_iter

Triple = Tuple[int, int, int]


@dataclass
class RankingMetrics:
    hits_at_10: float
    mean_rank: float
    count: int


@dataclass
class EntityPredictionMetrics:
    head_hits_at_10: float
    head_mean_rank: float
    tail_hits_at_10: float
    tail_mean_rank: float
    count: int


def evaluate_relation_prediction(
    model: nn.Module,
    triples: Sequence[Triple],
    rel_num: int,
    device: torch.device,
    batch_size: int = 128,
    filtered: bool = False,
    all_true_triples: set[Triple] | None = None,
) -> RankingMetrics:
    """Rank the gold relation among all relation candidates for each (h, t)."""

    model.eval()
    ranks: List[int] = []
    relation_filter = build_relation_filter(all_true_triples or set()) if filtered else {}

    with torch.no_grad():
        relation_candidates = torch.arange(rel_num, dtype=torch.long, device=device)
        for batch in batch_iter(triples, batch_size):
            batch_tensor = torch.tensor(batch, dtype=torch.long, device=device)
            h = batch_tensor[:, 0]
            t = batch_tensor[:, 1]
            gold_r = batch_tensor[:, 2]
            size = batch_tensor.size(0)

            scores = model.score(
                h.repeat_interleave(rel_num),
                relation_candidates.repeat(size),
                t.repeat_interleave(rel_num),
            ).view(size, rel_num)

            if filtered:
                for row, (h_id, t_id, r_id) in enumerate(batch):
                    for other_r in relation_filter.get((h_id, t_id), ()):
                        if other_r != r_id:
                            scores[row, other_r] = float("inf")

            target_scores = scores[torch.arange(size, device=device), gold_r]
            batch_ranks = 1 + (scores < target_scores.unsqueeze(1)).sum(dim=1)
            ranks.extend(batch_ranks.cpu().tolist())

    return summarize_ranks(ranks)


def evaluate_entity_prediction(
    model: nn.Module,
    triples: Sequence[Triple],
    ent_num: int,
    device: torch.device,
    batch_size: int = 32,
    candidate_chunk_size: int = 2048,
    filtered: bool = False,
    all_true_triples: set[Triple] | None = None,
    max_eval: int | None = None,
    progress_every: int = 0,
) -> EntityPredictionMetrics:
    """Rank the gold head and tail among all entity candidates."""

    model.eval()
    if max_eval is not None:
        if max_eval <= 0:
            raise ValueError("max_eval must be a positive integer.")
        triples = list(triples)[:max_eval]

    head_ranks: List[int] = []
    tail_ranks: List[int] = []
    head_filter, tail_filter = build_entity_filters(all_true_triples or set()) if filtered else ({}, {})
    total = len(triples)
    processed = 0
    next_progress = progress_every if progress_every > 0 else 0

    with torch.no_grad():
        for batch in batch_iter(triples, batch_size):
            batch_tensor = torch.tensor(batch, dtype=torch.long, device=device)
            h = batch_tensor[:, 0]
            t = batch_tensor[:, 1]
            r = batch_tensor[:, 2]
            head_ranks.extend(
                rank_entities(
                    model=model,
                    h=h,
                    r=r,
                    t=t,
                    ent_num=ent_num,
                    device=device,
                    candidate_chunk_size=candidate_chunk_size,
                    mode="head",
                    filtered=filtered,
                    filter_index=head_filter,
                    batch_triples=batch,
                )
            )
            tail_ranks.extend(
                rank_entities(
                    model=model,
                    h=h,
                    r=r,
                    t=t,
                    ent_num=ent_num,
                    device=device,
                    candidate_chunk_size=candidate_chunk_size,
                    mode="tail",
                    filtered=filtered,
                    filter_index=tail_filter,
                    batch_triples=batch,
                )
            )
            processed += len(batch)
            if progress_every > 0 and (processed >= next_progress or processed == total):
                print(f"entity_prediction progress={processed}/{total}")
                while next_progress <= processed:
                    next_progress += progress_every

    head = summarize_ranks(head_ranks)
    tail = summarize_ranks(tail_ranks)
    return EntityPredictionMetrics(
        head_hits_at_10=head.hits_at_10,
        head_mean_rank=head.mean_rank,
        tail_hits_at_10=tail.hits_at_10,
        tail_mean_rank=tail.mean_rank,
        count=head.count,
    )


def rank_entities(
    model: nn.Module,
    h: torch.Tensor,
    r: torch.Tensor,
    t: torch.Tensor,
    ent_num: int,
    device: torch.device,
    candidate_chunk_size: int,
    mode: str,
    filtered: bool,
    filter_index: Dict[Tuple[int, int], set[int]],
    batch_triples: Sequence[Triple],
) -> List[int]:
    size = h.size(0)
    target_scores = model.score(h, r, t)
    better_counts = torch.zeros(size, dtype=torch.long, device=device)

    for start in range(0, ent_num, candidate_chunk_size):
        end = min(start + candidate_chunk_size, ent_num)
        candidates = torch.arange(start, end, dtype=torch.long, device=device)
        chunk_size = candidates.size(0)

        candidate_grid = candidates.unsqueeze(0).expand(size, chunk_size).reshape(-1)
        if mode == "head":
            scores = model.score(
                candidate_grid,
                r.unsqueeze(1).expand(size, chunk_size).reshape(-1),
                t.unsqueeze(1).expand(size, chunk_size).reshape(-1),
            ).view(size, chunk_size)
        elif mode == "tail":
            scores = model.score(
                h.unsqueeze(1).expand(size, chunk_size).reshape(-1),
                r.unsqueeze(1).expand(size, chunk_size).reshape(-1),
                candidate_grid,
            ).view(size, chunk_size)
        else:
            raise ValueError("mode must be 'head' or 'tail'")

        if filtered:
            apply_entity_filter(scores, start, mode, filter_index, batch_triples)

        better_counts += (scores < target_scores.unsqueeze(1)).sum(dim=1)

    return (better_counts + 1).cpu().tolist()


def apply_entity_filter(
    scores: torch.Tensor,
    chunk_start: int,
    mode: str,
    filter_index: Dict[Tuple[int, int], set[int]],
    batch_triples: Sequence[Triple],
) -> None:
    chunk_end = chunk_start + scores.size(1)
    for row, (h_id, t_id, r_id) in enumerate(batch_triples):
        key = (r_id, t_id) if mode == "head" else (h_id, r_id)
        gold_entity = h_id if mode == "head" else t_id
        for entity in filter_index.get(key, ()):
            if entity != gold_entity and chunk_start <= entity < chunk_end:
                scores[row, entity - chunk_start] = float("inf")


def build_relation_filter(triples: Iterable[Triple]) -> Dict[Tuple[int, int], set[int]]:
    relation_filter: Dict[Tuple[int, int], set[int]] = defaultdict(set)
    for h, t, r in triples:
        relation_filter[(h, t)].add(r)
    return relation_filter


def build_entity_filters(
    triples: Iterable[Triple],
) -> Tuple[Dict[Tuple[int, int], set[int]], Dict[Tuple[int, int], set[int]]]:
    head_filter: Dict[Tuple[int, int], set[int]] = defaultdict(set)
    tail_filter: Dict[Tuple[int, int], set[int]] = defaultdict(set)
    for h, t, r in triples:
        head_filter[(r, t)].add(h)
        tail_filter[(h, r)].add(t)
    return head_filter, tail_filter


def summarize_ranks(ranks: Sequence[int]) -> RankingMetrics:
    if not ranks:
        return RankingMetrics(hits_at_10=0.0, mean_rank=0.0, count=0)
    count = len(ranks)
    hits = sum(1 for rank in ranks if rank <= 10) / count
    mean_rank = sum(ranks) / count
    return RankingMetrics(hits_at_10=hits, mean_rank=mean_rank, count=count)

from __future__ import annotations

from collections import defaultdict
from typing import Dict, List, Sequence, Tuple
import random

import torch

Triple = Tuple[int, int, int]


class NegativeSampler:
    """Uniform or Bernoulli head/tail corruption for KGE training."""

    def __init__(
        self,
        ent_num: int,
        train_triples: Sequence[Triple],
        all_true_triples: set[Triple] | None = None,
        mode: str = "unif",
        negative_size: int = 1,
        max_tries: int = 10,
    ) -> None:
        if mode not in {"unif", "bern"}:
            raise ValueError("mode must be 'unif' or 'bern'")
        self.ent_num = ent_num
        self.train_triples = list(train_triples)
        self.all_true_triples = all_true_triples or set()
        self.mode = mode
        self.negative_size = negative_size
        self.max_tries = max_tries
        self.head_prob = self._build_bern_probabilities()

    def sample(self, positive_batch: torch.Tensor) -> torch.Tensor:
        negatives: List[Triple] = []
        for h, t, r in positive_batch.detach().cpu().tolist():
            for _ in range(self.negative_size):
                negatives.append(self._sample_one(h, t, r))
        return torch.tensor(negatives, dtype=torch.long, device=positive_batch.device)

    def _sample_one(self, h: int, t: int, r: int) -> Triple:
        corrupt_head = self._should_corrupt_head(r)
        for _ in range(self.max_tries):
            candidate = random.randrange(self.ent_num)
            neg = (candidate, t, r) if corrupt_head else (h, candidate, r)
            if neg not in self.all_true_triples:
                return neg
        return neg

    def _should_corrupt_head(self, relation: int) -> bool:
        if self.mode == "unif":
            return random.random() < 0.5
        return random.random() < self.head_prob.get(relation, 0.5)

    def _build_bern_probabilities(self) -> Dict[int, float]:
        heads_by_rel: Dict[int, dict[int, set[int]]] = defaultdict(lambda: defaultdict(set))
        tails_by_rel: Dict[int, dict[int, set[int]]] = defaultdict(lambda: defaultdict(set))
        for h, t, r in self.train_triples:
            tails_by_rel[r][h].add(t)
            heads_by_rel[r][t].add(h)

        probabilities: Dict[int, float] = {}
        relations = set(heads_by_rel) | set(tails_by_rel)
        for r in relations:
            tails_per_head = average_set_size(tails_by_rel[r])
            heads_per_tail = average_set_size(heads_by_rel[r])
            denom = tails_per_head + heads_per_tail
            probabilities[r] = tails_per_head / denom if denom > 0 else 0.5
        return probabilities


def average_set_size(groups: dict[int, set[int]]) -> float:
    if not groups:
        return 0.0
    return sum(len(values) for values in groups.values()) / len(groups)


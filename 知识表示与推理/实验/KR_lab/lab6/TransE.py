from __future__ import annotations

import torch
from torch import nn
import torch.nn.functional as F

from kg_completion.cli import run_model_cli


class TransE(nn.Module):
    """TransE: h + r should be close to t in the same vector space."""

    def __init__(
        self,
        ent_num: int,
        rel_num: int,
        dim: int = 100,
        margin: float = 1.0,
        p_norm: int = 1,
    ) -> None:
        super().__init__()
        self.ent_num = ent_num
        self.rel_num = rel_num
        self.dim = dim
        self.margin = margin
        self.p_norm = p_norm

        self.ent_embeddings = nn.Embedding(ent_num, dim)
        self.rel_embeddings = nn.Embedding(rel_num, dim)
        self.reset_parameters()

    def reset_parameters(self) -> None:
        nn.init.xavier_uniform_(self.ent_embeddings.weight.data)
        nn.init.xavier_uniform_(self.rel_embeddings.weight.data)
        self.post_step()

    def score(self, h_idx: torch.Tensor, r_idx: torch.Tensor, t_idx: torch.Tensor) -> torch.Tensor:
        # Lower scores are better. The training loop applies margin ranking loss.
        h = self.ent_embeddings(h_idx)
        r = self.rel_embeddings(r_idx)
        t = self.ent_embeddings(t_idx)
        return torch.linalg.vector_norm(h + r - t, ord=self.p_norm, dim=1)

    def regularization(self, triples: torch.Tensor) -> torch.Tensor:
        # Keeps embeddings near the unit ball, following the common TransE constraint.
        h = self.ent_embeddings(triples[:, 0])
        r = self.rel_embeddings(triples[:, 2])
        t = self.ent_embeddings(triples[:, 1])
        ent_reg = ((h.norm(p=2, dim=1) - 1.0) ** 2).mean()
        tail_reg = ((t.norm(p=2, dim=1) - 1.0) ** 2).mean()
        rel_reg = (r**2).mean()
        return (ent_reg + tail_reg + rel_reg) / 3.0

    @torch.no_grad()
    def post_step(self) -> None:
        self.ent_embeddings.weight.data = F.normalize(self.ent_embeddings.weight.data, p=2, dim=1)


def main() -> None:
    run_model_cli("transe", TransE)


if __name__ == "__main__":
    main()


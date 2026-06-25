from __future__ import annotations

import torch
from torch import nn
import torch.nn.functional as F

from kg_completion.cli import run_model_cli


class TransH(nn.Module):
    """TransH: project entities onto a relation-specific hyperplane."""

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
        self.norm_embeddings = nn.Embedding(rel_num, dim)
        self.reset_parameters()

    def reset_parameters(self) -> None:
        nn.init.xavier_uniform_(self.ent_embeddings.weight.data)
        nn.init.xavier_uniform_(self.rel_embeddings.weight.data)
        nn.init.xavier_uniform_(self.norm_embeddings.weight.data)
        self.post_step()

    def _project(self, ent: torch.Tensor, norm: torch.Tensor) -> torch.Tensor:
        # e_perp = e - w^T e * w, where w is the relation hyperplane normal.
        norm = F.normalize(norm, p=2, dim=1)
        return ent - torch.sum(ent * norm, dim=1, keepdim=True) * norm

    def score(self, h_idx: torch.Tensor, r_idx: torch.Tensor, t_idx: torch.Tensor) -> torch.Tensor:
        h = self.ent_embeddings(h_idx)
        t = self.ent_embeddings(t_idx)
        r = self.rel_embeddings(r_idx)
        w = self.norm_embeddings(r_idx)
        h_proj = self._project(h, w)
        t_proj = self._project(t, w)
        return torch.linalg.vector_norm(h_proj + r - t_proj, ord=self.p_norm, dim=1)

    def regularization(self, triples: torch.Tensor) -> torch.Tensor:
        h = self.ent_embeddings(triples[:, 0])
        t = self.ent_embeddings(triples[:, 1])
        r = self.rel_embeddings(triples[:, 2])
        w = F.normalize(self.norm_embeddings(triples[:, 2]), p=2, dim=1)

        ent_reg = (((h.norm(p=2, dim=1) - 1.0) ** 2).mean() + ((t.norm(p=2, dim=1) - 1.0) ** 2).mean()) / 2
        orthogonal_reg = torch.sum(r * w, dim=1).pow(2).mean()
        return ent_reg + orthogonal_reg

    @torch.no_grad()
    def post_step(self) -> None:
        self.ent_embeddings.weight.data = F.normalize(self.ent_embeddings.weight.data, p=2, dim=1)
        self.norm_embeddings.weight.data = F.normalize(self.norm_embeddings.weight.data, p=2, dim=1)


def main() -> None:
    run_model_cli("transh", TransH)


if __name__ == "__main__":
    main()


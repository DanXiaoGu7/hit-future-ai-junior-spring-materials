from __future__ import annotations

import torch
from torch import nn
import torch.nn.functional as F

from kg_completion.cli import run_model_cli


class TransR(nn.Module):
    """TransR: map entities into each relation space before translation."""

    def __init__(
        self,
        ent_num: int,
        rel_num: int,
        ent_dim: int = 100,
        rel_dim: int = 100,
        margin: float = 1.0,
        p_norm: int = 1,
    ) -> None:
        super().__init__()
        self.ent_num = ent_num
        self.rel_num = rel_num
        self.ent_dim = ent_dim
        self.rel_dim = rel_dim
        self.margin = margin
        self.p_norm = p_norm

        self.ent_embeddings = nn.Embedding(ent_num, ent_dim)
        self.rel_embeddings = nn.Embedding(rel_num, rel_dim)
        self.transfer_matrices = nn.Embedding(rel_num, ent_dim * rel_dim)
        self.reset_parameters()

    def reset_parameters(self) -> None:
        nn.init.xavier_uniform_(self.ent_embeddings.weight.data)
        nn.init.xavier_uniform_(self.rel_embeddings.weight.data)

        # Start from an identity-like projection for stable early training.
        matrices = torch.zeros(self.rel_num, self.ent_dim, self.rel_dim)
        diag = min(self.ent_dim, self.rel_dim)
        matrices[:, torch.arange(diag), torch.arange(diag)] = 1.0
        self.transfer_matrices.weight.data.copy_(matrices.view(self.rel_num, -1))
        self.post_step()

    def _project(self, ent: torch.Tensor, r_idx: torch.Tensor) -> torch.Tensor:
        matrix = self.transfer_matrices(r_idx).view(-1, self.ent_dim, self.rel_dim)
        return torch.bmm(ent.unsqueeze(1), matrix).squeeze(1)

    def score(self, h_idx: torch.Tensor, r_idx: torch.Tensor, t_idx: torch.Tensor) -> torch.Tensor:
        h = self.ent_embeddings(h_idx)
        t = self.ent_embeddings(t_idx)
        r = self.rel_embeddings(r_idx)
        h_proj = self._project(h, r_idx)
        t_proj = self._project(t, r_idx)
        return torch.linalg.vector_norm(h_proj + r - t_proj, ord=self.p_norm, dim=1)

    def regularization(self, triples: torch.Tensor) -> torch.Tensor:
        h = self.ent_embeddings(triples[:, 0])
        t = self.ent_embeddings(triples[:, 1])
        r = self.rel_embeddings(triples[:, 2])
        matrix = self.transfer_matrices(triples[:, 2]).view(-1, self.ent_dim, self.rel_dim)
        h_proj = self._project(h, triples[:, 2])
        t_proj = self._project(t, triples[:, 2])

        ent_reg = (((h.norm(p=2, dim=1) - 1.0) ** 2).mean() + ((t.norm(p=2, dim=1) - 1.0) ** 2).mean()) / 2
        proj_reg = (((h_proj.norm(p=2, dim=1) - 1.0) ** 2).mean() + ((t_proj.norm(p=2, dim=1) - 1.0) ** 2).mean()) / 2
        rel_reg = (r**2).mean()
        identity = torch.zeros_like(matrix)
        diag = min(self.ent_dim, self.rel_dim)
        idx = torch.arange(diag, device=matrix.device)
        identity[:, idx, idx] = 1.0
        matrix_reg = ((matrix - identity) ** 2).mean()
        return (ent_reg + proj_reg + rel_reg + matrix_reg) / 4.0

    @torch.no_grad()
    def post_step(self) -> None:
        self.ent_embeddings.weight.data = F.normalize(self.ent_embeddings.weight.data, p=2, dim=1)
        self.rel_embeddings.weight.data = F.normalize(self.rel_embeddings.weight.data, p=2, dim=1)
        matrices = self.transfer_matrices.weight.data.view(self.rel_num, -1)
        max_norm = float(min(self.ent_dim, self.rel_dim) ** 0.5)
        norms = matrices.norm(p=2, dim=1, keepdim=True).clamp_min(1e-12)
        scale = (max_norm / norms).clamp(max=1.0)
        matrices.mul_(scale)


def main() -> None:
    run_model_cli("transr", TransR)


if __name__ == "__main__":
    main()

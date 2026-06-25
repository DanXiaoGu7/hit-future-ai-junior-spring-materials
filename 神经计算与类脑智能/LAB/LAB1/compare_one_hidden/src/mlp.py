"""
MLP network for MNIST classification
"""

from torch import nn


class MLP(nn.Module):
    def __init__(
        self,
        input_dim=28 * 28,
        hidden_dim=512,
        num_classes=10,
        dropout=0.2,
    ):
        super().__init__()
        self.network = nn.Sequential(
            nn.Flatten(),
            nn.Linear(input_dim, hidden_dim),
            nn.ReLU(),
            nn.Dropout(dropout),
            nn.Linear(hidden_dim, num_classes),
        )

    def forward(self, x):
        return self.network(x)

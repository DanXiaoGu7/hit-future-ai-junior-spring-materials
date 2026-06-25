"""
Produce the MNIST dataloader
"""

from pathlib import Path

from torch.utils.data import DataLoader
from torchvision import datasets, transforms


def create_dataloader(data_path, batch_size=512, train=True, download=False, num_workers=0):
    data_path = Path(data_path)
    transform = transforms.ToTensor()

    dataset = datasets.MNIST(
        root=str(data_path),
        train=train,
        download=download,
        transform=transform,
    )

    dataloader = DataLoader(
        dataset,
        batch_size=batch_size,
        shuffle=train,
        num_workers=num_workers,
    )
    return dataloader

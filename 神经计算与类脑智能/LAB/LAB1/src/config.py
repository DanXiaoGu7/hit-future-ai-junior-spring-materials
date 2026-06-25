"""
Network config setting, will be used in train.py and eval.py
"""

from dataclasses import dataclass


@dataclass(frozen=True)
class MnistConfig:
    num_classes: int = 10
    image_height: int = 28
    image_width: int = 28
    hidden_dim_1: int = 512
    hidden_dim_2: int = 256
    dropout: float = 0.2
    lr: float = 0.01
    momentum: float = 0.9
    epoch_size: int = 10
    batch_size: int = 64
    test_batch_size: int = 1000
    optimizer: str = "sgd"
    device_target: str = "AUTO"
    seed: int = 42
    num_workers: int = 0
    log_interval: int = 100


mnist_cfg = MnistConfig()

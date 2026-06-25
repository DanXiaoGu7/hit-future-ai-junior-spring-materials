"""
Network config setting, will be used in train.py and eval.py
"""

from dataclasses import dataclass


@dataclass(frozen=True)
class MnistConfig:
    num_classes: int = 10
    image_height: int = 28
    image_width: int = 28
    batch_size: int = 512
    eval_batch_size: int = 512
    num_memories_per_class: int = 10
    kmeans_iters: int = 8
    threshold: float = 0.4
    recall_steps: int = 0
    device_target: str = "AUTO"
    seed: int = 42
    num_workers: int = 0


mnist_cfg = MnistConfig()

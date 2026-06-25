from __future__ import annotations

import json
import random
from pathlib import Path
from typing import Any

import numpy as np
import torch


def ensure_dir(path: str | Path) -> Path:
    directory = Path(path)
    directory.mkdir(parents=True, exist_ok=True)
    return directory


def load_json(path: str | Path) -> dict[str, Any]:
    return json.loads(Path(path).read_text(encoding="utf-8"))


def dump_json(data: Any, path: str | Path) -> None:
    Path(path).write_text(json.dumps(data, indent=2, ensure_ascii=False), encoding="utf-8")


def load_jsonl(path: str | Path) -> list[dict[str, Any]]:
    records: list[dict[str, Any]] = []
    with Path(path).open("r", encoding="utf-8") as handle:
        for line in handle:
            line = line.strip()
            if not line:
                continue
            records.append(json.loads(line))
    return records


def save_jsonl(records: list[dict[str, Any]], path: str | Path) -> None:
    with Path(path).open("w", encoding="utf-8") as handle:
        for record in records:
            handle.write(json.dumps(record, ensure_ascii=False) + "\n")


def append_jsonl(record: dict[str, Any], path: str | Path) -> None:
    with Path(path).open("a", encoding="utf-8") as handle:
        handle.write(json.dumps(record, ensure_ascii=False) + "\n")


def set_seed(seed: int) -> None:
    random.seed(seed)
    np.random.seed(seed)
    torch.manual_seed(seed)
    if torch.cuda.is_available():
        torch.cuda.manual_seed_all(seed)


def parse_torch_dtype(use_fp16: bool) -> torch.dtype | None:
    if not torch.cuda.is_available():
        return None
    return torch.float16 if use_fp16 else None


def normalize_prompt(prompt: str) -> str:
    prompt = prompt.strip()
    if "Human:" in prompt and prompt.rstrip().endswith("Assistant:"):
        return prompt
    return f"\n\nHuman: {prompt}\n\nAssistant:"


def build_full_text(prompt: str, response: str) -> str:
    normalized_prompt = normalize_prompt(prompt)
    response = response.strip()
    if not response:
        return normalized_prompt
    return f"{normalized_prompt} {response}".rstrip()


def scalarize(value: Any) -> float:
    if isinstance(value, torch.Tensor):
        if value.numel() == 1:
            return float(value.detach().cpu().item())
        return float(value.detach().cpu().mean().item())
    if isinstance(value, (int, float)):
        return float(value)
    if hasattr(value, "item"):
        return float(value.item())
    if isinstance(value, (list, tuple)) and value:
        return float(sum(scalarize(item) for item in value) / len(value))
    raise TypeError(f"Cannot convert value of type {type(value)!r} to float.")

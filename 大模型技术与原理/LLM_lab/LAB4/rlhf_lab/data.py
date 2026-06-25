from __future__ import annotations

import random
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from torch.utils.data import Dataset

from .utils import build_full_text, normalize_prompt


ASSISTANT_MARKER = "\n\nAssistant:"


def _longest_common_prefix(left: str, right: str) -> str:
    limit = min(len(left), len(right))
    index = 0
    while index < limit and left[index] == right[index]:
        index += 1
    return left[:index]


def split_last_assistant_turn(transcript: str) -> tuple[str, str]:
    index = transcript.rfind(ASSISTANT_MARKER)
    if index == -1:
        fallback_marker = "Assistant:"
        index = transcript.rfind(fallback_marker)
        if index == -1:
            raise ValueError("Could not find the final assistant turn in transcript.")
        prompt = transcript[: index + len(fallback_marker)].strip()
        response = transcript[index + len(fallback_marker) :].strip()
        return prompt, response

    prompt = transcript[: index + len(ASSISTANT_MARKER)].strip()
    response = transcript[index + len(ASSISTANT_MARKER) :].strip()
    return prompt, response


def parse_hh_preference_pair(chosen_text: str, rejected_text: str) -> dict[str, str]:
    chosen_prompt, chosen_response = split_last_assistant_turn(chosen_text)
    rejected_prompt, rejected_response = split_last_assistant_turn(rejected_text)

    if chosen_prompt == rejected_prompt:
        prompt = chosen_prompt
    else:
        common_prefix = _longest_common_prefix(chosen_text, rejected_text)
        marker_index = common_prefix.rfind(ASSISTANT_MARKER)
        if marker_index == -1:
            raise ValueError("Could not align chosen/rejected transcripts into a shared prompt.")
        prompt = chosen_text[: marker_index + len(ASSISTANT_MARKER)].strip()
        chosen_response = chosen_text[marker_index + len(ASSISTANT_MARKER) :].strip()
        rejected_response = rejected_text[marker_index + len(ASSISTANT_MARKER) :].strip()

    return {
        "prompt": normalize_prompt(prompt),
        "chosen": chosen_response,
        "rejected": rejected_response,
    }


def build_processed_records(raw_examples: list[dict[str, Any]]) -> list[dict[str, str]]:
    processed: list[dict[str, str]] = []
    for example in raw_examples:
        try:
            record = parse_hh_preference_pair(example["chosen"], example["rejected"])
        except Exception:
            continue
        if not record["chosen"] or not record["rejected"]:
            continue
        processed.append(record)
    return processed


def split_for_lab(
    records: list[dict[str, str]],
    max_rm_train_samples: int,
    max_rm_eval_samples: int,
    max_ppo_prompts: int,
    seed: int,
) -> tuple[list[dict[str, str]], list[dict[str, str]], list[dict[str, str]]]:
    rng = random.Random(seed)
    shuffled = list(records)
    rng.shuffle(shuffled)

    rm_train = shuffled[:max_rm_train_samples]
    rm_eval = shuffled[max_rm_train_samples : max_rm_train_samples + max_rm_eval_samples]

    ppo_candidates = rm_train if len(rm_train) <= max_ppo_prompts else rng.sample(rm_train, max_ppo_prompts)
    ppo_prompts = [{"prompt": item["prompt"]} for item in ppo_candidates]
    return rm_train, rm_eval, ppo_prompts


@dataclass
class PairwiseRewardDataset(Dataset):
    records: list[dict[str, str]]
    tokenizer: Any
    max_length: int

    def __len__(self) -> int:
        return len(self.records)

    def __getitem__(self, index: int) -> dict[str, Any]:
        record = self.records[index]
        chosen_text = build_full_text(record["prompt"], record["chosen"])
        rejected_text = build_full_text(record["prompt"], record["rejected"])

        chosen_features = self.tokenizer(
            chosen_text,
            truncation=True,
            max_length=self.max_length,
            add_special_tokens=True,
        )
        rejected_features = self.tokenizer(
            rejected_text,
            truncation=True,
            max_length=self.max_length,
            add_special_tokens=True,
        )

        return {
            "chosen_input_ids": chosen_features["input_ids"],
            "chosen_attention_mask": chosen_features["attention_mask"],
            "rejected_input_ids": rejected_features["input_ids"],
            "rejected_attention_mask": rejected_features["attention_mask"],
        }


@dataclass
class PromptDataset(Dataset):
    records: list[dict[str, str]]
    tokenizer: Any
    max_prompt_length: int

    def __len__(self) -> int:
        return len(self.records)

    def __getitem__(self, index: int) -> dict[str, Any]:
        prompt = normalize_prompt(self.records[index]["prompt"])
        tokenized = self.tokenizer(
            prompt,
            truncation=True,
            max_length=self.max_prompt_length,
            add_special_tokens=True,
        )
        return {
            "prompt": prompt,
            "input_ids": tokenized["input_ids"],
        }


def prompt_data_collator(features: list[dict[str, Any]]) -> dict[str, list[Any]]:
    return {
        "prompt": [feature["prompt"] for feature in features],
        "input_ids": [feature["input_ids"] for feature in features],
    }

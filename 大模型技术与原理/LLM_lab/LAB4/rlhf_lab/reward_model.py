from __future__ import annotations

from typing import Any

import numpy as np
import torch
import torch.nn.functional as F
from transformers import Trainer


def compute_ranking_loss(reward_chosen: torch.Tensor, reward_rejected: torch.Tensor) -> torch.Tensor:
    return -F.logsigmoid(reward_chosen - reward_rejected).mean()


class PairwiseDataCollator:
    def __init__(self, tokenizer: Any) -> None:
        self.tokenizer = tokenizer

    def __call__(self, features: list[dict[str, Any]]) -> dict[str, torch.Tensor]:
        chosen_features = [
            {
                "input_ids": feature["chosen_input_ids"],
                "attention_mask": feature["chosen_attention_mask"],
            }
            for feature in features
        ]
        rejected_features = [
            {
                "input_ids": feature["rejected_input_ids"],
                "attention_mask": feature["rejected_attention_mask"],
            }
            for feature in features
        ]

        chosen_batch = self.tokenizer.pad(chosen_features, padding=True, return_tensors="pt")
        rejected_batch = self.tokenizer.pad(rejected_features, padding=True, return_tensors="pt")

        return {
            "input_ids_chosen": chosen_batch["input_ids"],
            "attention_mask_chosen": chosen_batch["attention_mask"],
            "input_ids_rejected": rejected_batch["input_ids"],
            "attention_mask_rejected": rejected_batch["attention_mask"],
        }


class RewardTrainer(Trainer):
    def compute_loss(
        self,
        model: torch.nn.Module,
        inputs: dict[str, torch.Tensor],
        return_outputs: bool = False,
    ) -> torch.Tensor | tuple[torch.Tensor, dict[str, torch.Tensor]]:
        chosen_rewards = model(
            input_ids=inputs["input_ids_chosen"],
            attention_mask=inputs["attention_mask_chosen"],
        ).logits.squeeze(-1)

        rejected_rewards = model(
            input_ids=inputs["input_ids_rejected"],
            attention_mask=inputs["attention_mask_rejected"],
        ).logits.squeeze(-1)

        loss = compute_ranking_loss(chosen_rewards, rejected_rewards)

        if not return_outputs:
            return loss

        outputs = {
            "chosen_rewards": chosen_rewards,
            "rejected_rewards": rejected_rewards,
        }
        return loss, outputs

    def prediction_step(
        self,
        model: torch.nn.Module,
        inputs: dict[str, torch.Tensor],
        prediction_loss_only: bool,
        ignore_keys: list[str] | None = None,
    ) -> tuple[torch.Tensor | None, torch.Tensor | None, torch.Tensor | None]:
        prepared_inputs = self._prepare_inputs(inputs)
        with torch.no_grad():
            loss, outputs = self.compute_loss(model, prepared_inputs, return_outputs=True)

        if prediction_loss_only:
            return loss.detach(), None, None

        logits = torch.stack([outputs["chosen_rewards"], outputs["rejected_rewards"]], dim=-1)
        labels = torch.zeros(logits.size(0), dtype=torch.long, device=logits.device)
        return loss.detach(), logits.detach(), labels


def compute_reward_metrics(eval_prediction: Any) -> dict[str, float]:
    logits, _labels = eval_prediction
    if isinstance(logits, tuple):
        logits = logits[0]
    chosen_rewards = logits[:, 0]
    rejected_rewards = logits[:, 1]
    accuracy = np.mean(chosen_rewards > rejected_rewards)
    margin = np.mean(chosen_rewards - rejected_rewards)
    return {
        "accuracy": float(accuracy),
        "margin": float(margin),
    }

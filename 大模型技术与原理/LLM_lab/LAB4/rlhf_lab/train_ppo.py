from __future__ import annotations

import argparse
import inspect
from pathlib import Path
from typing import Any

import torch
from transformers import AutoModelForSequenceClassification, AutoTokenizer
from trl import AutoModelForCausalLMWithValueHead, PPOConfig, PPOTrainer, create_reference_model

from .data import PromptDataset, prompt_data_collator
from .utils import (
    append_jsonl,
    build_full_text,
    ensure_dir,
    load_json,
    load_jsonl,
    normalize_prompt,
    parse_torch_dtype,
    scalarize,
    set_seed,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run PPO alignment for the RLHF lab.")
    parser.add_argument("--config", required=True, help="Path to the PPO config JSON file.")
    return parser.parse_args()


def build_ppo_config(raw_config: dict[str, Any]) -> PPOConfig:
    signature = inspect.signature(PPOConfig.__init__)
    kwargs: dict[str, Any] = {
        "model_name": raw_config["model_name"],
        "learning_rate": float(raw_config["learning_rate"]),
        "batch_size": int(raw_config["batch_size"]),
        "mini_batch_size": int(raw_config["mini_batch_size"]),
        "gradient_accumulation_steps": int(raw_config["gradient_accumulation_steps"]),
        "ppo_epochs": int(raw_config["ppo_epochs"]),
        "seed": int(raw_config["seed"]),
        "log_with": None,
    }
    if "init_kl_coef" in signature.parameters:
        kwargs["init_kl_coef"] = float(raw_config["init_kl_coef"])
    if "kl_penalty" in signature.parameters:
        kwargs["kl_penalty"] = raw_config["kl_penalty"]
    if "target_kl" in signature.parameters:
        kwargs["target_kl"] = float(raw_config["target_kl"])
    elif "target" in signature.parameters:
        kwargs["target"] = float(raw_config["target_kl"])
    return PPOConfig(**kwargs)


def score_with_reward_model(
    texts: list[str],
    reward_model,
    reward_tokenizer,
    scoring_device: torch.device,
    output_device: torch.device,
    max_length: int,
) -> list[torch.Tensor]:
    tokenized = reward_tokenizer(
        texts,
        padding=True,
        truncation=True,
        max_length=max_length,
        return_tensors="pt",
    )
    tokenized = {key: value.to(scoring_device) for key, value in tokenized.items()}
    with torch.no_grad():
        scores = reward_model(**tokenized).logits.squeeze(-1)
    return [torch.tensor(score.item(), device=output_device) for score in scores]


def save_policy_checkpoint(model, tokenizer, output_dir: Path, name: str) -> None:
    checkpoint_dir = ensure_dir(output_dir / name)
    model.pretrained_model.save_pretrained(checkpoint_dir)
    tokenizer.save_pretrained(checkpoint_dir)


def main() -> None:
    args = parse_args()
    config = load_json(args.config)
    set_seed(int(config["seed"]))

    output_dir = ensure_dir(config["output_dir"])
    metrics_path = output_dir / "ppo_metrics.jsonl"
    dtype = parse_torch_dtype(bool(config.get("fp16", False)))
    metrics_path.write_text("", encoding="utf-8")

    tokenizer = AutoTokenizer.from_pretrained(config["model_name"], trust_remote_code=True)
    if tokenizer.pad_token is None:
        tokenizer.pad_token = tokenizer.eos_token

    prompt_records = load_jsonl(config["prompt_file"])
    dataset = PromptDataset(prompt_records, tokenizer, int(config["max_prompt_length"]))

    actor_model_kwargs = {"trust_remote_code": True}
    if dtype is not None:
        actor_model_kwargs["torch_dtype"] = dtype

    actor_model = AutoModelForCausalLMWithValueHead.from_pretrained(
        config["model_name"],
        **actor_model_kwargs,
    )
    actor_model.pretrained_model.config.pad_token_id = tokenizer.pad_token_id
    actor_model.pretrained_model.config.use_cache = False
    ref_model = create_reference_model(actor_model)

    ppo_config = build_ppo_config(config)
    trainer = PPOTrainer(
        config=ppo_config,
        model=actor_model,
        ref_model=ref_model,
        tokenizer=tokenizer,
        dataset=dataset,
        data_collator=prompt_data_collator,
    )

    trainer_device = trainer.accelerator.device
    reward_device_name = config.get("reward_device", "cuda" if torch.cuda.is_available() else "cpu")
    if reward_device_name == "cuda" and not torch.cuda.is_available():
        reward_device_name = "cpu"
    reward_device = torch.device(reward_device_name)

    reward_tokenizer = AutoTokenizer.from_pretrained(config["reward_model_path"], trust_remote_code=True)
    if reward_tokenizer.pad_token is None:
        reward_tokenizer.pad_token = reward_tokenizer.eos_token

    reward_model_kwargs = {
        "num_labels": 1,
        "trust_remote_code": True,
    }
    if dtype is not None and reward_device.type == "cuda":
        reward_model_kwargs["torch_dtype"] = dtype

    reward_model = AutoModelForSequenceClassification.from_pretrained(
        config["reward_model_path"],
        **reward_model_kwargs,
    )
    reward_model.eval()
    reward_model.to(reward_device)

    generation_kwargs = {
    "do_sample": True,
    "temperature": float(config["temperature"]),
    "top_p": float(config["top_p"]),
    "max_new_tokens": int(config["max_new_tokens"]),
    "pad_token_id": tokenizer.pad_token_id,
    "eos_token_id": tokenizer.eos_token_id,
    "remove_invalid_values": True,
    "renormalize_logits": True,
}




    max_steps = int(config["max_steps"])
    save_every_steps = int(config["save_every_steps"])

    for step, batch in enumerate(trainer.dataloader, start=1):
        query_tensors = [torch.tensor(ids, device=trainer_device) for ids in batch["input_ids"]]
        response_tensors = trainer.generate(query_tensors, return_prompt=False, **generation_kwargs)

        if isinstance(response_tensors, torch.Tensor):
            response_tensors = [response_tensors[index] for index in range(response_tensors.size(0))]

        responses = tokenizer.batch_decode(response_tensors, skip_special_tokens=True)
        reward_texts = [
            build_full_text(normalize_prompt(prompt), response)
            for prompt, response in zip(batch["prompt"], responses)
        ]
        rewards = score_with_reward_model(
            reward_texts,
            reward_model,
            reward_tokenizer,
            scoring_device=reward_device,
            output_device=trainer_device,
            max_length=int(config["reward_max_length"]),
        )

        stats = trainer.step(query_tensors, response_tensors, rewards)
        record = {
            "step": step,
            "mean_reward": sum(reward.item() for reward in rewards) / len(rewards),
            "objective/kl": scalarize(stats.get("objective/kl", 0.0)),
            "ppo/loss/policy": scalarize(stats.get("ppo/loss/policy", 0.0)),
            "ppo/loss/value": scalarize(stats.get("ppo/loss/value", 0.0)),
        }
        append_jsonl(record, metrics_path)

        print(
            f"step={step} "
            f"reward={record['mean_reward']:.4f} "
            f"kl={record['objective/kl']:.4f}"
        )

        if step % save_every_steps == 0:
            save_policy_checkpoint(trainer.model, tokenizer, output_dir, f"checkpoint-step-{step}")

        if step >= max_steps:
            break

    save_policy_checkpoint(trainer.model, tokenizer, output_dir, "final_policy")
    print(f"Saved PPO policy to {output_dir / 'final_policy'}")


if __name__ == "__main__":
    main()

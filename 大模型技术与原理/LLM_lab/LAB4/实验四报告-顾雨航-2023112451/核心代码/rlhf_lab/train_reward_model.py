from __future__ import annotations

import argparse
from peft import LoraConfig, PeftModel, TaskType, get_peft_model
from transformers import AutoModelForSequenceClassification, AutoTokenizer, TrainingArguments

from .data import PairwiseRewardDataset
from .reward_model import PairwiseDataCollator, RewardTrainer, compute_reward_metrics
from .utils import dump_json, ensure_dir, load_json, load_jsonl, parse_torch_dtype, set_seed


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Train the reward model for RLHF.")
    parser.add_argument("--config", required=True, help="Path to the RM config JSON file.")
    return parser.parse_args()


def build_lora_config(config: dict) -> LoraConfig:
    return LoraConfig(
        task_type=TaskType.SEQ_CLS,
        r=int(config["lora_r"]),
        lora_alpha=int(config["lora_alpha"]),
        lora_dropout=float(config["lora_dropout"]),
        target_modules=list(config["target_modules"]),
    )


def maybe_save_merged_model(model, output_dir) -> None:
    final_dir = ensure_dir(output_dir / "final_model")
    if isinstance(model, PeftModel):
        merged_model = model.merge_and_unload()
        merged_model.save_pretrained(final_dir)
    else:
        model.save_pretrained(final_dir)


def main() -> None:
    args = parse_args()
    config = load_json(args.config)
    set_seed(int(config["seed"]))

    output_dir = ensure_dir(config["output_dir"])
    dtype = parse_torch_dtype(bool(config.get("fp16", False)))

    tokenizer = AutoTokenizer.from_pretrained(config["model_name"], trust_remote_code=True)
    if tokenizer.pad_token is None:
        tokenizer.pad_token = tokenizer.eos_token

    model_kwargs = {
        "num_labels": 1,
        "trust_remote_code": True,
    }
    if dtype is not None:
        model_kwargs["torch_dtype"] = dtype

    model = AutoModelForSequenceClassification.from_pretrained(
        config["model_name"],
        **model_kwargs,
    )
    model.config.pad_token_id = tokenizer.pad_token_id
    model.config.use_cache = False

    if bool(config.get("use_lora", False)):
        model = get_peft_model(model, build_lora_config(config))

    train_records = load_jsonl(config["train_file"])
    eval_records = load_jsonl(config["eval_file"])

    train_dataset = PairwiseRewardDataset(train_records, tokenizer, int(config["max_length"]))
    eval_dataset = PairwiseRewardDataset(eval_records, tokenizer, int(config["max_length"]))
    data_collator = PairwiseDataCollator(tokenizer)

    training_args = TrainingArguments(
        output_dir=str(output_dir),
        learning_rate=float(config["learning_rate"]),
        per_device_train_batch_size=int(config["per_device_train_batch_size"]),
        per_device_eval_batch_size=int(config["per_device_eval_batch_size"]),
        gradient_accumulation_steps=int(config["gradient_accumulation_steps"]),
        num_train_epochs=float(config["num_train_epochs"]),
        weight_decay=float(config["weight_decay"]),
        warmup_ratio=float(config["warmup_ratio"]),
        logging_steps=int(config["logging_steps"]),
        evaluation_strategy="steps",
        eval_steps=int(config["eval_steps"]),
        save_strategy="steps",
        save_steps=int(config["save_steps"]),
        save_total_limit=int(config["save_total_limit"]),
        load_best_model_at_end=True,
        metric_for_best_model="accuracy",
        greater_is_better=True,
        remove_unused_columns=False,
        fp16=bool(config.get("fp16", False)),
        report_to="none",
        seed=int(config["seed"]),
    )

    trainer = RewardTrainer(
        model=model,
        args=training_args,
        train_dataset=train_dataset,
        eval_dataset=eval_dataset,
        data_collator=data_collator,
        tokenizer=tokenizer,
        compute_metrics=compute_reward_metrics,
    )

    trainer.train()
    eval_metrics = trainer.evaluate()

    maybe_save_merged_model(trainer.model, output_dir)
    tokenizer.save_pretrained(output_dir / "final_model")

    dump_json(trainer.state.log_history, output_dir / "rm_log_history.json")
    dump_json(eval_metrics, output_dir / "eval_metrics.json")

    print(f"Reward model saved to {output_dir / 'final_model'}")
    print(f"Evaluation metrics saved to {output_dir / 'eval_metrics.json'}")


if __name__ == "__main__":
    main()

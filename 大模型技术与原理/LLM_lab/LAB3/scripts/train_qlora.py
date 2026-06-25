from __future__ import annotations

import argparse
import json
import os
from pathlib import Path

import torch
from datasets import load_dataset
from peft import LoraConfig, TaskType, get_peft_model, prepare_model_for_kbit_training
from torch.nn.utils.rnn import pad_sequence
from transformers import (
    AutoModelForCausalLM,
    BitsAndBytesConfig,
    Trainer,
    TrainingArguments,
    set_seed,
)

from common import format_user_message, load_tokenizer, pick_torch_dtype, save_json, str2bool


os.environ.setdefault("TOKENIZERS_PARALLELISM", "false")


class SupervisedDataCollator:
    def __init__(self, pad_token_id: int):
        self.pad_token_id = pad_token_id

    def __call__(self, features):
        input_ids = [torch.tensor(sample["input_ids"], dtype=torch.long) for sample in features]
        labels = [torch.tensor(sample["labels"], dtype=torch.long) for sample in features]
        attention_masks = [
            torch.tensor(sample["attention_mask"], dtype=torch.long) for sample in features
        ]
        return {
            "input_ids": pad_sequence(
                input_ids, batch_first=True, padding_value=self.pad_token_id
            ),
            "labels": pad_sequence(labels, batch_first=True, padding_value=-100),
            "attention_mask": pad_sequence(
                attention_masks, batch_first=True, padding_value=0
            ),
        }


def parse_args():
    parser = argparse.ArgumentParser(description="Run a QLoRA experiment for LAB3.")
    parser.add_argument(
        "--model_name_or_path",
        default="Qwen/Qwen2.5-7B-Instruct",
        help="Base model path or Hugging Face model id.",
    )
    parser.add_argument(
        "--train_file",
        default="data/domain_dataset_sample.json",
        help="JSON or JSONL file with instruction-input-output samples.",
    )
    parser.add_argument(
        "--output_dir",
        default="outputs/qwen2.5-7b-edu-lora",
        help="Directory to store adapter weights and logs.",
    )
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--max_seq_length", type=int, default=512)
    parser.add_argument("--per_device_train_batch_size", type=int, default=1)
    parser.add_argument("--gradient_accumulation_steps", type=int, default=8)
    parser.add_argument("--learning_rate", type=float, default=2e-4)
    parser.add_argument("--weight_decay", type=float, default=0.01)
    parser.add_argument("--warmup_ratio", type=float, default=0.03)
    parser.add_argument("--logging_steps", type=int, default=10)
    parser.add_argument("--save_steps", type=int, default=100)
    parser.add_argument("--max_steps", type=int, default=200)
    parser.add_argument("--num_train_epochs", type=float, default=1.0)
    parser.add_argument("--lora_r", type=int, default=16)
    parser.add_argument("--lora_alpha", type=int, default=32)
    parser.add_argument("--lora_dropout", type=float, default=0.05)
    parser.add_argument("--use_4bit", type=str2bool, default=True)
    parser.add_argument("--trust_remote_code", type=str2bool, default=False)
    return parser.parse_args()


def count_parameters(model) -> dict:
    trainable = sum(param.numel() for param in model.parameters() if param.requires_grad)
    total = sum(param.numel() for param in model.parameters())
    ratio = (trainable / total * 100.0) if total else 0.0
    return {
        "trainable_params": int(trainable),
        "total_params": int(total),
        "trainable_ratio_percent": round(ratio, 4),
    }


def preprocess_dataset(dataset, tokenizer, max_seq_length: int):
    if not tokenizer.chat_template:
        raise ValueError(
            "Tokenizer does not provide a chat template. Please choose a chat model such as Qwen2.5 or Llama-3-Instruct."
        )

    def _preprocess(example):
        system_prompt = (
            example.get("system")
            or "You are a careful domain assistant. Answer based on the provided context and stay concise."
        )
        user_message = format_user_message(example["instruction"], example.get("input", ""))
        assistant_message = example["output"].strip()

        prompt_messages = [
            {"role": "system", "content": system_prompt},
            {"role": "user", "content": user_message},
        ]
        full_messages = prompt_messages + [
            {"role": "assistant", "content": assistant_message}
        ]

        prompt_text = tokenizer.apply_chat_template(
            prompt_messages,
            tokenize=False,
            add_generation_prompt=True,
        )
        full_text = tokenizer.apply_chat_template(
            full_messages,
            tokenize=False,
            add_generation_prompt=False,
        )

        prompt_ids = tokenizer(prompt_text, add_special_tokens=False).input_ids
        full_ids = tokenizer(full_text, add_special_tokens=False).input_ids
        labels = [-100] * len(prompt_ids) + full_ids[len(prompt_ids) :]

        input_ids = full_ids[:max_seq_length]
        labels = labels[:max_seq_length]
        attention_mask = [1] * len(input_ids)
        active_label_count = sum(1 for label in labels if label != -100)

        return {
            "input_ids": input_ids,
            "labels": labels,
            "attention_mask": attention_mask,
            "active_label_count": active_label_count,
        }

    processed = dataset.map(
        _preprocess,
        remove_columns=dataset.column_names,
        desc="Tokenizing dataset",
    )
    processed = processed.filter(
        lambda example: example["active_label_count"] > 0,
        desc="Filtering empty supervision samples",
    )
    return processed.remove_columns(["active_label_count"])


def main():
    args = parse_args()
    set_seed(args.seed)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    tokenizer = load_tokenizer(
        args.model_name_or_path,
        trust_remote_code=args.trust_remote_code,
    )
    torch_dtype = pick_torch_dtype()

    model_kwargs = {
        "trust_remote_code": args.trust_remote_code,
        "device_map": "auto",
        "low_cpu_mem_usage": True,
    }
    if args.use_4bit:
        model_kwargs["quantization_config"] = BitsAndBytesConfig(
            load_in_4bit=True,
            bnb_4bit_quant_type="nf4",
            bnb_4bit_use_double_quant=True,
            bnb_4bit_compute_dtype=torch_dtype,
        )
    else:
        model_kwargs["torch_dtype"] = torch_dtype

    model = AutoModelForCausalLM.from_pretrained(
        args.model_name_or_path,
        **model_kwargs,
    )
    model.config.use_cache = False

    if args.use_4bit:
        model = prepare_model_for_kbit_training(model)

    lora_config = LoraConfig(
        r=args.lora_r,
        lora_alpha=args.lora_alpha,
        target_modules=["q_proj", "k_proj", "v_proj", "o_proj"],
        lora_dropout=args.lora_dropout,
        bias="none",
        task_type=TaskType.CAUSAL_LM,
    )
    model = get_peft_model(model, lora_config)
    parameter_stats = count_parameters(model)
    print(json.dumps(parameter_stats, ensure_ascii=False, indent=2))

    dataset = load_dataset("json", data_files={"train": args.train_file})["train"]
    train_dataset = preprocess_dataset(dataset, tokenizer, args.max_seq_length)

    training_args = TrainingArguments(
        output_dir=str(output_dir),
        overwrite_output_dir=True,
        per_device_train_batch_size=args.per_device_train_batch_size,
        gradient_accumulation_steps=args.gradient_accumulation_steps,
        learning_rate=args.learning_rate,
        weight_decay=args.weight_decay,
        warmup_ratio=args.warmup_ratio,
        logging_steps=args.logging_steps,
        save_steps=args.save_steps,
        save_strategy="steps",
        max_steps=args.max_steps,
        num_train_epochs=args.num_train_epochs,
        lr_scheduler_type="cosine",
        report_to="none",
        fp16=(torch.cuda.is_available() and torch_dtype == torch.float16),
        bf16=(torch.cuda.is_available() and torch_dtype == torch.bfloat16),
        gradient_checkpointing=True,
        remove_unused_columns=False,
        optim="paged_adamw_8bit" if args.use_4bit else "adamw_torch",
    )

    trainer = Trainer(
        model=model,
        args=training_args,
        train_dataset=train_dataset,
        data_collator=SupervisedDataCollator(tokenizer.pad_token_id),
    )

    if torch.cuda.is_available():
        torch.cuda.empty_cache()
        torch.cuda.reset_peak_memory_stats()

    train_result = trainer.train()
    trainer.save_model()
    tokenizer.save_pretrained(output_dir)
    trainer.state.save_to_json(str(output_dir / "trainer_state.json"))

    metrics = dict(train_result.metrics)
    if torch.cuda.is_available():
        metrics["max_gpu_allocated_mb"] = round(
            torch.cuda.max_memory_allocated() / 1024 / 1024, 2
        )
        metrics["max_gpu_reserved_mb"] = round(
            torch.cuda.max_memory_reserved() / 1024 / 1024, 2
        )

    summary = {
        "base_model": args.model_name_or_path,
        "train_file": args.train_file,
        "dataset_size": len(train_dataset),
        "use_4bit": args.use_4bit,
        "torch_dtype": str(torch_dtype).replace("torch.", ""),
        "lora": {
            "r": args.lora_r,
            "alpha": args.lora_alpha,
            "dropout": args.lora_dropout,
            "target_modules": ["q_proj", "k_proj", "v_proj", "o_proj"],
        },
        "training_args": {
            "learning_rate": args.learning_rate,
            "per_device_train_batch_size": args.per_device_train_batch_size,
            "gradient_accumulation_steps": args.gradient_accumulation_steps,
            "max_seq_length": args.max_seq_length,
            "max_steps": args.max_steps,
        },
        "parameter_stats": parameter_stats,
        "train_metrics": metrics,
    }
    save_json(output_dir / "metrics_summary.json", summary)
    print(json.dumps(summary, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()

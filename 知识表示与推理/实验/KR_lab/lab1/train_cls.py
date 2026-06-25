import argparse
from typing import Dict, List

import numpy as np
import evaluate
from datasets import DownloadMode, load_dataset
from transformers import (
    AutoTokenizer,
    DataCollatorWithPadding,
    Trainer,
    TrainingArguments,
    set_seed,
)

from BERT import DistilBertForSequenceClassification


id2label: Dict[int, str] = {0: "NEGATIVE", 1: "POSITIVE"}
label2id: Dict[str, int] = {"NEGATIVE": 0, "POSITIVE": 1}

# Reuse local cache to avoid repeated downloads in offline environments.
tokenizer = AutoTokenizer.from_pretrained("./cache/distilbert")
accuracy = evaluate.load("accuracy", cache_dir="./cache")


def compute_metrics(eval_pred):
    predictions, labels = eval_pred
    predictions = np.argmax(predictions, axis=1)
    res = accuracy.compute(predictions=predictions, references=labels)
    print(res)
    return res


def tokenize_batch(batch: Dict[str, List[str]]):
    return tokenizer(batch["sentence"], truncation=True)


def build_datasets():
    raw = load_dataset(
        "stanfordnlp/sst2",
        cache_dir="./cache",
        download_mode=DownloadMode.REUSE_DATASET_IF_EXISTS,
    )
    tokenized = raw.map(tokenize_batch, batched=True, remove_columns=["sentence"])
    tokenized = tokenized.rename_column("label", "labels")
    return tokenized


def train_and_eval(args):
    set_seed(args.seed)
    datasets = build_datasets()

    model = DistilBertForSequenceClassification.from_pretrained(
        "./cache/distilbert", num_labels=2, id2label=id2label, label2id=label2id
    )
    # Toggle pooling strategy via CLI: cls / mean / max
    model.config.pooling_type = args.pooling

    data_collator = DataCollatorWithPadding(tokenizer=tokenizer)

    training_args = TrainingArguments(
        output_dir=f"./ckpt/cls_{args.pooling}",
        learning_rate=args.lr,
        per_device_train_batch_size=args.batch_size,
        per_device_eval_batch_size=max(args.batch_size * 2, 32),
        num_train_epochs=args.epochs,
        weight_decay=0.01,
        evaluation_strategy="epoch",
        save_strategy="epoch",
        load_best_model_at_end=True,
        metric_for_best_model="accuracy",
        greater_is_better=True,
        logging_strategy="steps",
        logging_steps=50,
        report_to="none",
        save_total_limit=2,
        seed=args.seed,
    )

    trainer = Trainer(
        model=model,
        args=training_args,
        train_dataset=datasets["train"],
        eval_dataset=datasets["validation"],
        tokenizer=tokenizer,
        data_collator=data_collator,
        compute_metrics=compute_metrics,
    )

    trainer.train()
    eval_res = trainer.evaluate()
    print(f"Validation accuracy with {args.pooling} pooling: {eval_res.get('eval_accuracy'):.4f}")
    return eval_res


def parse_args():
    parser = argparse.ArgumentParser(description="Fine-tune DistilBERT on SST-2 with configurable pooling.")
    parser.add_argument("--pooling", type=str, choices=["cls", "mean", "max"], default="mean")
    parser.add_argument("--epochs", type=int, default=3)
    parser.add_argument("--batch_size", type=int, default=32)
    parser.add_argument("--lr", type=float, default=2e-5)
    parser.add_argument("--seed", type=int, default=42)
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    train_and_eval(args)

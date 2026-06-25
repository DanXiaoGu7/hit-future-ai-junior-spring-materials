from __future__ import annotations

import argparse

from datasets import load_dataset

from .data import build_processed_records, split_for_lab
from .utils import ensure_dir, load_json, save_jsonl, set_seed


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Prepare preference data for the RLHF lab.")
    parser.add_argument("--config", required=True, help="Path to the data config JSON file.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    config = load_json(args.config)
    set_seed(int(config["seed"]))

    output_dir = ensure_dir(config["output_dir"])

    dataset = load_dataset(config["dataset_name"], split=config["dataset_split"])
    raw_examples = [dataset[index] for index in range(len(dataset))]
    processed_records = build_processed_records(raw_examples)

    rm_train, rm_eval, ppo_prompts = split_for_lab(
        processed_records,
        max_rm_train_samples=int(config["max_rm_train_samples"]),
        max_rm_eval_samples=int(config["max_rm_eval_samples"]),
        max_ppo_prompts=int(config["max_ppo_prompts"]),
        seed=int(config["seed"]),
    )

    save_jsonl(rm_train, output_dir / "rm_train.jsonl")
    save_jsonl(rm_eval, output_dir / "rm_eval.jsonl")
    save_jsonl(ppo_prompts, output_dir / "ppo_prompts.jsonl")

    print(f"Saved {len(rm_train)} RM training pairs to {output_dir / 'rm_train.jsonl'}")
    print(f"Saved {len(rm_eval)} RM validation pairs to {output_dir / 'rm_eval.jsonl'}")
    print(f"Saved {len(ppo_prompts)} PPO prompts to {output_dir / 'ppo_prompts.jsonl'}")


if __name__ == "__main__":
    main()

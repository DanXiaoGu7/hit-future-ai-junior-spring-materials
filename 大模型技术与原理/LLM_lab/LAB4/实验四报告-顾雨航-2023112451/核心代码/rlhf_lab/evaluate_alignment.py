from __future__ import annotations

import argparse
from pathlib import Path

import torch
from transformers import AutoModelForCausalLM, AutoTokenizer

from .utils import ensure_dir, load_json, load_jsonl, normalize_prompt, save_jsonl, set_seed


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Evaluate base vs aligned model responses.")
    parser.add_argument("--config", required=True, help="Path to the evaluation config JSON file.")
    return parser.parse_args()


def load_generation_model(model_name_or_path: str, use_fp16: bool):
    tokenizer = AutoTokenizer.from_pretrained(model_name_or_path, trust_remote_code=True)
    if tokenizer.pad_token is None:
        tokenizer.pad_token = tokenizer.eos_token

    model_kwargs = {"trust_remote_code": True}
    if torch.cuda.is_available() and use_fp16:
        model_kwargs["torch_dtype"] = torch.float16

    model = AutoModelForCausalLM.from_pretrained(
        model_name_or_path,
        **model_kwargs,
    )
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    model.to(device)
    model.eval()
    return model, tokenizer, device


def generate_response(model, tokenizer, device, prompt: str, max_new_tokens: int, temperature: float, top_p: float) -> str:
    formatted_prompt = normalize_prompt(prompt)
    tokenized = tokenizer(formatted_prompt, return_tensors="pt").to(device)
    with torch.no_grad():
        output = model.generate(
            **tokenized,
            do_sample=True,
            temperature=temperature,
            top_p=top_p,
            max_new_tokens=max_new_tokens,
            pad_token_id=tokenizer.pad_token_id,
        )
    generated_tokens = output[0][tokenized["input_ids"].shape[-1] :]
    return tokenizer.decode(generated_tokens, skip_special_tokens=True).strip()


def write_markdown_case_study(records: list[dict], path: Path) -> None:
    lines = ["# Alignment Case Study", ""]
    for index, record in enumerate(records, start=1):
        lines.extend(
            [
                f"## Case {index}",
                f"- Category: {record['category']}",
                f"- Prompt: {record['prompt']}",
                "",
                "### Base Model",
                record["base_response"],
                "",
                "### Aligned Model",
                record["aligned_response"],
                "",
            ]
        )
    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    args = parse_args()
    config = load_json(args.config)
    set_seed(int(config["seed"]))
    output_dir = ensure_dir(config["output_dir"])

    prompts = load_jsonl(config["prompts_file"])
    base_model, base_tokenizer, base_device = load_generation_model(
        config["base_model_name"],
        bool(config.get("fp16", False)),
    )
    aligned_model, aligned_tokenizer, aligned_device = load_generation_model(
        config["aligned_model_path"],
        bool(config.get("fp16", False)),
    )

    comparison_records: list[dict] = []
    for item in prompts:
        prompt = item["prompt"]
        base_response = generate_response(
            base_model,
            base_tokenizer,
            base_device,
            prompt,
            max_new_tokens=int(config["max_new_tokens"]),
            temperature=float(config["temperature"]),
            top_p=float(config["top_p"]),
        )
        aligned_response = generate_response(
            aligned_model,
            aligned_tokenizer,
            aligned_device,
            prompt,
            max_new_tokens=int(config["max_new_tokens"]),
            temperature=float(config["temperature"]),
            top_p=float(config["top_p"]),
        )
        comparison_records.append(
            {
                "category": item["category"],
                "prompt": prompt,
                "base_response": base_response,
                "aligned_response": aligned_response,
            }
        )

    save_jsonl(comparison_records, output_dir / "alignment_cases.jsonl")
    write_markdown_case_study(comparison_records, output_dir / "case_study.md")

    print(f"Saved alignment cases to {output_dir / 'alignment_cases.jsonl'}")
    print(f"Saved markdown case study to {output_dir / 'case_study.md'}")


if __name__ == "__main__":
    main()

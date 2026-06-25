from __future__ import annotations

import argparse

from peft import PeftModel
from transformers import AutoModelForCausalLM

from common import load_tokenizer, pick_torch_dtype, str2bool


def parse_args():
    parser = argparse.ArgumentParser(description="Merge LoRA adapter into the base model.")
    parser.add_argument(
        "--base_model",
        default="Qwen/Qwen2.5-7B-Instruct",
        help="Base model path or Hugging Face model id.",
    )
    parser.add_argument(
        "--adapter_path",
        default="outputs/qwen2.5-7b-edu-lora",
        help="Directory of the trained LoRA adapter.",
    )
    parser.add_argument(
        "--output_dir",
        default="outputs/qwen2.5-7b-edu-lora-merged",
        help="Where to save the merged model.",
    )
    parser.add_argument("--trust_remote_code", type=str2bool, default=False)
    return parser.parse_args()


def main():
    args = parse_args()
    torch_dtype = pick_torch_dtype()

    base_model = AutoModelForCausalLM.from_pretrained(
        args.base_model,
        torch_dtype=torch_dtype,
        low_cpu_mem_usage=True,
        device_map="cpu",
        trust_remote_code=args.trust_remote_code,
    )
    model = PeftModel.from_pretrained(base_model, args.adapter_path)
    merged_model = model.merge_and_unload()
    merged_model.save_pretrained(args.output_dir, safe_serialization=True)

    tokenizer = load_tokenizer(args.base_model, trust_remote_code=args.trust_remote_code)
    tokenizer.save_pretrained(args.output_dir)
    print(f"Merged model saved to: {args.output_dir}")


if __name__ == "__main__":
    main()


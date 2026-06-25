from __future__ import annotations

import argparse
import json

import torch
from transformers import AutoModelForCausalLM, BitsAndBytesConfig

from common import pick_torch_dtype, save_json, str2bool


def parse_args():
    parser = argparse.ArgumentParser(description="Measure GPU memory for quantized/non-quantized loading.")
    parser.add_argument(
        "--model_name_or_path",
        default="Qwen/Qwen2.5-7B-Instruct",
        help="Model path or Hugging Face model id.",
    )
    parser.add_argument("--use_4bit", type=str2bool, default=True)
    parser.add_argument("--trust_remote_code", type=str2bool, default=False)
    parser.add_argument(
        "--output_file",
        default="outputs/memory_observation.json",
        help="Where to save the observation result.",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    torch_dtype = pick_torch_dtype()
    result = {
        "model_name_or_path": args.model_name_or_path,
        "use_4bit": args.use_4bit,
        "torch_dtype": str(torch_dtype).replace("torch.", ""),
        "cuda_available": torch.cuda.is_available(),
    }

    try:
        if torch.cuda.is_available():
            torch.cuda.empty_cache()
            torch.cuda.reset_peak_memory_stats()

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
        model.eval()

        if torch.cuda.is_available():
            torch.cuda.synchronize()
            result["status"] = "success"
            result["gpu_memory"] = {
                "allocated_mb": round(torch.cuda.memory_allocated() / 1024 / 1024, 2),
                "reserved_mb": round(torch.cuda.memory_reserved() / 1024 / 1024, 2),
                "max_allocated_mb": round(
                    torch.cuda.max_memory_allocated() / 1024 / 1024, 2
                ),
                "max_reserved_mb": round(
                    torch.cuda.max_memory_reserved() / 1024 / 1024, 2
                ),
            }
        else:
            result["status"] = "success"
            result["gpu_memory"] = None

        parameter_count = sum(param.numel() for param in model.parameters())
        result["total_params"] = int(parameter_count)
    except Exception as exc:
        result["status"] = "error"
        result["error"] = str(exc)

    save_json(args.output_file, result)
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()


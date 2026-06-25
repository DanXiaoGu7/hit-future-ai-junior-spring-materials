from __future__ import annotations

import argparse
import json
from pathlib import Path

import torch
from peft import PeftModel
from transformers import AutoModelForCausalLM, BitsAndBytesConfig

from common import load_tokenizer, pick_torch_dtype, str2bool


def parse_args():
    parser = argparse.ArgumentParser(
        description="Run Base vs LoRA-Tuned case studies without launching Gradio."
    )
    parser.add_argument(
        "--base_model",
        default="Qwen/Qwen2.5-7B-Instruct",
        help="Base model id or local path.",
    )
    parser.add_argument(
        "--adapter_path",
        default="outputs/qwen2.5-7b-edu-lora",
        help="Path to LoRA adapter.",
    )
    parser.add_argument(
        "--cases_file",
        default="data/eval_cases.json",
        help="Path to evaluation cases.",
    )
    parser.add_argument(
        "--output_file",
        default="outputs/case_study_results.md",
        help="Markdown file used to save side-by-side comparisons.",
    )
    parser.add_argument("--use_4bit", type=str2bool, default=True)
    parser.add_argument("--trust_remote_code", type=str2bool, default=False)
    parser.add_argument("--max_cases", type=int, default=3)
    parser.add_argument("--max_new_tokens", type=int, default=128)
    parser.add_argument("--temperature", type=float, default=0.0)
    parser.add_argument("--top_p", type=float, default=0.9)
    parser.add_argument(
        "--system_prompt",
        default="You are a careful domain assistant. Answer directly and mention limits when uncertain.",
    )
    return parser.parse_args()


def load_cases(path: str, max_cases: int) -> list[dict]:
    payload = json.loads(Path(path).read_text(encoding="utf-8"))
    if not isinstance(payload, list):
        raise ValueError("cases_file must contain a JSON list.")
    return payload[:max_cases]


def build_question(case: dict) -> str:
    instruction = case.get("instruction", "").strip()
    input_text = case.get("input", "").strip()
    if instruction and input_text:
        return f"{instruction}\n\n{input_text}"
    return instruction or input_text


def load_base_model(args):
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
    model = AutoModelForCausalLM.from_pretrained(args.base_model, **model_kwargs)
    model.eval()
    return model


def load_tuned_model(args):
    base_model = load_base_model(args)
    model = PeftModel.from_pretrained(base_model, args.adapter_path)
    model.eval()
    return model


def generate_answer(tokenizer, model, question: str, args) -> str:
    messages = [
        {"role": "system", "content": args.system_prompt},
        {"role": "user", "content": question},
    ]
    prompt_text = tokenizer.apply_chat_template(
        messages,
        tokenize=False,
        add_generation_prompt=True,
    )
    device = next(model.parameters()).device
    inputs = tokenizer(prompt_text, return_tensors="pt").to(device)

    generate_kwargs = {
        "max_new_tokens": args.max_new_tokens,
        "pad_token_id": tokenizer.pad_token_id,
        "eos_token_id": tokenizer.eos_token_id,
    }
    if args.temperature > 0:
        generate_kwargs["do_sample"] = True
        generate_kwargs["temperature"] = args.temperature
        generate_kwargs["top_p"] = args.top_p
    else:
        generate_kwargs["do_sample"] = False

    with torch.inference_mode():
        outputs = model.generate(**inputs, **generate_kwargs)
    generated_tokens = outputs[0][inputs["input_ids"].shape[-1] :]
    return tokenizer.decode(generated_tokens, skip_special_tokens=True).strip()


def release_model(model):
    del model
    if torch.cuda.is_available():
        torch.cuda.empty_cache()


def render_markdown(results: list[dict]) -> str:
    lines = [
        "# Case Study Results",
        "",
        "| Case | Question | Base Model | LoRA-Tuned Model |",
        "| --- | --- | --- | --- |",
    ]
    for item in results:
        question = item["question"].replace("\n", "<br>")
        base_answer = item["base_answer"].replace("\n", "<br>")
        tuned_answer = item["tuned_answer"].replace("\n", "<br>")
        lines.append(
            f"| {item['case_id']} | {question} | {base_answer} | {tuned_answer} |"
        )
    lines.append("")
    return "\n".join(lines)


def main():
    args = parse_args()
    cases = load_cases(args.cases_file, args.max_cases)
    tokenizer = load_tokenizer(args.base_model, trust_remote_code=args.trust_remote_code)

    print("Loading base model...")
    base_model = load_base_model(args)
    results = []
    for index, case in enumerate(cases, start=1):
        question = build_question(case)
        print(f"[Base] Running case {index}/{len(cases)}")
        base_answer = generate_answer(tokenizer, base_model, question, args)
        results.append(
            {
                "case_id": index,
                "question": question,
                "base_answer": base_answer,
                "tuned_answer": "",
            }
        )
    release_model(base_model)

    print("Loading LoRA-tuned model...")
    tuned_model = load_tuned_model(args)
    for item in results:
        print(f"[Tuned] Running case {item['case_id']}/{len(results)}")
        item["tuned_answer"] = generate_answer(tokenizer, tuned_model, item["question"], args)
    release_model(tuned_model)

    output_path = Path(args.output_file)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(render_markdown(results), encoding="utf-8")

    json_path = output_path.with_suffix(".json")
    json_path.write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding="utf-8")

    print(f"Saved markdown results to: {output_path}")
    print(f"Saved JSON results to: {json_path}")


if __name__ == "__main__":
    main()

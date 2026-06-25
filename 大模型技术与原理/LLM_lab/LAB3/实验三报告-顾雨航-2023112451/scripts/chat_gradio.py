from __future__ import annotations

import argparse

import gradio as gr
import torch
from peft import PeftModel
from transformers import AutoModelForCausalLM, BitsAndBytesConfig

from common import load_tokenizer, pick_torch_dtype, str2bool


def parse_args():
    parser = argparse.ArgumentParser(description="Launch a Gradio chat demo for the tuned model.")
    parser.add_argument(
        "--merged_model_path",
        default="",
        help="Path to merged model. If set, base_model and adapter_path are ignored.",
    )
    parser.add_argument(
        "--base_model",
        default="Qwen/Qwen2.5-7B-Instruct",
        help="Base model id when loading adapter only.",
    )
    parser.add_argument(
        "--adapter_path",
        default="",
        help="Path to LoRA adapter when not using merged_model_path.",
    )
    parser.add_argument("--use_4bit", type=str2bool, default=True)
    parser.add_argument("--trust_remote_code", type=str2bool, default=False)
    parser.add_argument("--max_new_tokens", type=int, default=256)
    parser.add_argument("--temperature", type=float, default=0.7)
    parser.add_argument("--top_p", type=float, default=0.9)
    parser.add_argument(
        "--system_prompt",
        default="You are a careful domain assistant. Answer directly and mention limits when uncertain.",
    )
    parser.add_argument("--server_name", default="127.0.0.1")
    parser.add_argument("--server_port", type=int, default=7860)
    parser.add_argument("--share", type=str2bool, default=False)
    parser.add_argument("--inbrowser", type=str2bool, default=False)
    return parser.parse_args()


def load_model(args):
    torch_dtype = pick_torch_dtype()
    if args.merged_model_path:
        tokenizer = load_tokenizer(
            args.merged_model_path,
            trust_remote_code=args.trust_remote_code,
        )
        model = AutoModelForCausalLM.from_pretrained(
            args.merged_model_path,
            torch_dtype=torch_dtype,
            device_map="auto",
            trust_remote_code=args.trust_remote_code,
            low_cpu_mem_usage=True,
        )
        model.eval()
        return tokenizer, model

    if not args.adapter_path:
        raise ValueError("adapter_path is required when merged_model_path is not provided.")

    tokenizer = load_tokenizer(args.base_model, trust_remote_code=args.trust_remote_code)
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

    base_model = AutoModelForCausalLM.from_pretrained(args.base_model, **model_kwargs)
    model = PeftModel.from_pretrained(base_model, args.adapter_path)
    model.eval()
    return tokenizer, model


def normalize_history(history):
    messages = []
    for item in history:
        if isinstance(item, dict):
            role = item.get("role")
            content = item.get("content")
            if role in {"user", "assistant"} and content:
                messages.append({"role": role, "content": content})
        elif isinstance(item, (list, tuple)) and len(item) == 2:
            user_msg, assistant_msg = item
            if user_msg:
                messages.append({"role": "user", "content": user_msg})
            if assistant_msg:
                messages.append({"role": "assistant", "content": assistant_msg})
    return messages


def main():
    args = parse_args()
    tokenizer, model = load_model(args)

    def predict(message, history):
        history = history or []
        messages = [{"role": "system", "content": args.system_prompt}]
        messages.extend(normalize_history(history))
        messages.append({"role": "user", "content": message})

        prompt_text = tokenizer.apply_chat_template(
            messages,
            tokenize=False,
            add_generation_prompt=True,
        )
        device = next(model.parameters()).device
        inputs = tokenizer(prompt_text, return_tensors="pt").to(device)
        with torch.inference_mode():
            outputs = model.generate(
                **inputs,
                max_new_tokens=args.max_new_tokens,
                do_sample=True,
                temperature=args.temperature,
                top_p=args.top_p,
                pad_token_id=tokenizer.pad_token_id,
                eos_token_id=tokenizer.eos_token_id,
            )

        generated_tokens = outputs[0][inputs["input_ids"].shape[-1] :]
        return tokenizer.decode(generated_tokens, skip_special_tokens=True).strip()

    demo = gr.ChatInterface(
        fn=predict,
        title="LAB3 LoRA Demo",
        description="Ask a domain question and inspect the tuned model response.",
    )
    demo.launch(
        server_name=args.server_name,
        server_port=args.server_port,
        share=args.share,
        inbrowser=args.inbrowser,
    )


if __name__ == "__main__":
    main()

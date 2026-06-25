# 实验 3：垂直领域模型微调实践

本目录按照实验手册要求，补齐了一套最小可运行实验工程，覆盖以下四项：

1. 领域数据集构建与模板化
2. QLoRA 微调
3. 参数规模 / 显存 / Loss 记录
4. 模型合并与 Gradio 交互展示

当前推荐直接选用中文能力较稳定、且满足手册“7B+ 模型”要求的 `Qwen/Qwen2.5-7B-Instruct`。

## 1. 目录说明

```text
LAB3/
├─ data/
│  ├─ domain_dataset_sample.json   # 垂直领域样例数据
│  └─ eval_cases.json              # Case Study 样例问题
├─ scripts/
│  ├─ common.py
│  ├─ measure_memory.py            # 量化/非量化显存观测
│  ├─ train_qlora.py               # QLoRA 训练
│  ├─ plot_loss.py                 # Loss 曲线绘制
│  ├─ merge_lora.py                # 合并 LoRA 权重
│  └─ chat_gradio.py               # Web 对话界面
├─ requirements.txt
├─ 实验报告模板.md
└─ 实验3：垂直领域模型微调实践.pdf
```

## 2. 推荐环境

你当前机器检测到的是 `RTX 4060 Laptop 8GB`。这足够完成手册中的 `4-bit + LoRA` 微调实验，但建议按下面的思路执行：

- 主实验：`Qwen/Qwen2.5-7B-Instruct + 4-bit QLoRA`
- 调试实验：先用少量样本和较短步数验证流程
- 非量化对比：可以先尝试同一 7B 模型的 `fp16` 加载；如果直接 OOM，也可以将其作为“不开启量化无法在 8GB 显存运行”的实验结论

注意：

- 你当前 PowerShell 环境中 `python` 不在 PATH 里，运行前需要先进入你自己的 Python/conda/venv 环境。
- `bitsandbytes` 在原生 Windows 上可能存在兼容问题。如果安装或加载 4-bit 量化失败，优先切到 `WSL2 / Linux / AutoDL / Colab`，不要在原生 Windows 上长时间排障。

## 3. 安装依赖

必须使用 Python 3.10 或 3.11。若使用 Python 3.9，`scipy`、`bitsandbytes` 等依赖可能无法安装。

另外，`requirements.txt` 不包含 `torch`，请先确保当前环境里已经安装了可用的 GPU 版 PyTorch，再执行：

```bash
pip install -r requirements.txt
```

如果你在 Windows 上遇到 4-bit 量化加载崩溃，优先做法是：

- 新建一个干净的 conda 环境
- 避免直接在已有的旧环境里覆盖安装
- 优先使用较稳定的 Transformers 4.x 依赖组合

## 4. 数据集要求

手册要求将数据整理成“指令 - 输入 - 输出”格式。样例文件是 [data/domain_dataset_sample.json](/d:/A2026CSLAB/LLM_lab/LAB3/data/domain_dataset_sample.json:1)，格式如下：

```json
[
  {
    "system": "你是某高校教务与实验管理助手，回答必须准确、分点、避免编造。",
    "instruction": "回答高校教务相关问题",
    "input": "学生因生病错过期末考试，应如何申请缓考？",
    "output": "..."
  }
]
```

建议你的最终实验数据至少做到：

- 有明确垂直领域边界，例如高校教务、医疗问答、法律咨询、金融客服
- 至少 100 条以上高质量样本
- 表达风格统一
- 输出尽量结构化，便于微调收敛

当前目录还提供了一份可直接用于课程实验的扩充版数据集：[data/domain_dataset_edu_expanded.json](/d:/A2026CSLAB/LLM_lab/LAB3/data/domain_dataset_edu_expanded.json:1)。
如果你暂时没有自己的数据，建议先用这份扩充版完成实验流程和报告。

## 5. 实验执行顺序

### 第一步：显存对比

先记录量化和非量化的显存表现，对应手册“显存与微调效率观测”。

4-bit 量化加载：

```bash
python scripts/measure_memory.py --model_name_or_path Qwen/Qwen2.5-7B-Instruct --use_4bit true --output_file outputs/qwen7b_4bit_memory.json
```

非量化加载：

```bash
python scripts/measure_memory.py --model_name_or_path Qwen/Qwen2.5-7B-Instruct --use_4bit false --output_file outputs/qwen7b_fp16_memory.json
```

注意：这里输出文件名写成 `fp16` 只是为了区分“非量化加载”，实际运行时的精度以 JSON 文件里的 `torch_dtype` 字段为准。在支持 BF16 的显卡上，结果可能显示为 `bfloat16`。

如果第二条命令直接 OOM，也属于有效实验现象，可写入报告。

### 第二步：QLoRA 微调

推荐先用样例数据跑通流程，再换成你自己的最终数据集。对于 8GB 显存设备，建议第一次先跑 `100` 步确认训练稳定，再把 `max_steps` 提高到 `200` 作为正式结果。

```bash
python scripts/train_qlora.py --model_name_or_path Qwen/Qwen2.5-7B-Instruct --train_file data/domain_dataset_edu_expanded.json --output_dir outputs/qwen2.5-7b-edu-lora --use_4bit true --lora_r 16 --lora_alpha 32 --learning_rate 2e-4 --max_seq_length 512 --per_device_train_batch_size 1 --gradient_accumulation_steps 8 --max_steps 100
```

8GB 显存下的推荐起步参数：

- `per_device_train_batch_size=1`
- `gradient_accumulation_steps=8`
- `max_seq_length=512`
- `lora_r=16`
- `lora_alpha=32`
- `max_steps=100~200`

### 第三步：绘制 Loss 曲线

```bash
python scripts/plot_loss.py --trainer_state outputs/qwen2.5-7b-edu-lora/trainer_state.json --output_file outputs/qwen2.5-7b-edu-lora/loss_curve.png
```

### 第四步：合并 LoRA 权重

```bash
python scripts/merge_lora.py --base_model Qwen/Qwen2.5-7B-Instruct --adapter_path outputs/qwen2.5-7b-edu-lora --output_dir outputs/qwen2.5-7b-edu-lora-merged
```

### 第五步：启动 Web 展示

使用合并后的模型：

```bash
python scripts/chat_gradio.py --merged_model_path outputs/qwen2.5-7b-edu-lora-merged
```

如果本机环境提示 `localhost is not accessible`，改用：

```bash
python scripts/chat_gradio.py --merged_model_path outputs/qwen2.5-7b-edu-lora-merged --share true
```

如果启动后出现 `TypeError: unhashable type: 'dict'`，通常不是模型错误，而是 `gradio / fastapi / starlette / jinja2` 版本不兼容。优先在当前环境中重装以下依赖：

```bash
pip install --upgrade --force-reinstall "gradio==4.44.1" "fastapi<1.0" "starlette<1.0" "jinja2<4.0"
```

如果随后又出现 `ImportError: cannot import name 'HfFolder' from 'huggingface_hub'`，说明当前环境把 `huggingface_hub` 升到了 `1.x`，而 `gradio 4.44.x` 仍依赖旧接口。此时继续执行：

```bash
pip install --upgrade --force-reinstall "huggingface_hub<1.0"
```

如果 `share=True` 之后又提示 `Could not create share link`，说明当前网络无法连通 Gradio 的 share server，需要切换网络，或者改为仅本地访问。

或者使用基座模型 + Adapter：

```bash
python scripts/chat_gradio.py --base_model Qwen/Qwen2.5-7B-Instruct --adapter_path outputs/qwen2.5-7b-edu-lora --use_4bit true
```

## 6. 你需要记录的结果

严格对齐手册，最终报告至少要有：

- 基座模型名称
- LoRA 超参数：`r`、`alpha`、`dropout`
- 学习率、batch size、累积步数、最大步数
- 可训练参数量与占比
- 量化与非量化显存对比
- Loss 曲线图
- 至少 3 组 `Base vs LoRA-Tuned` Case Study
- 对 `LoRA Rank` 大小影响的讨论

其中：

- 可训练参数和比例会自动写入 `output_dir/metrics_summary.json`
- 训练日志会由 `Trainer` 自动保存
- Loss 曲线图由 `plot_loss.py` 生成

## 7. 建议你的答辩表达方式

如果老师问“为什么要用 LoRA / QLoRA”，可以直接从三点回答：

1. 全参数微调对显存要求过高，不适合普通 8GB~16GB 显卡。
2. LoRA 只训练少量低秩增量矩阵，可显著降低可训练参数比例。
3. QLoRA 通过 4-bit 量化进一步降低模型加载显存，使 7B 级模型在消费级显卡上也能做微调实验。

## 8. 交付前自查

- 训练是否真的跑完并生成 Adapter
- `metrics_summary.json` 是否包含参数占比和显存峰值
- `loss_curve.png` 是否可读
- Web 页面是否能正常回答垂直领域问题
- 实验报告是否包含对比表和分析讨论

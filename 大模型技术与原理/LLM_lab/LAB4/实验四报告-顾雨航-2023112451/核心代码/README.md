# 实验四：基于 RLHF 的大语言模型价值观对齐实验

本仓库提供一个精简版 RLHF 实验框架，对应实验指导书中的完整流程：

1. 从 `Anthropic/hh-rlhf` 构建偏好数据；
2. 使用 Pairwise Ranking Loss 训练奖励模型（Reward Model, RM）；
3. 使用 PPO 对策略模型进行价值观对齐；
4. 对比对齐前后的回答，绘制报告所需曲线。

实验目标是理解 RLHF 如何把人类偏好转化为可优化的奖励信号，并观察 PPO 训练中 `Mean Reward` 与 `KL Divergence` 的变化。最终报告需要说明模型配置、展示曲线，并提供至少 3 组对齐前后的 Case Study。

## 项目结构

```text
configs/
  data_config.json       # 数据处理配置
  rm_config.json         # 奖励模型训练配置
  ppo_config.json        # PPO 对齐配置
  eval_config.json       # 对齐效果评估配置
data/
  eval_prompts.jsonl     # 固定评估提示词
  processed/             # 预处理后的训练数据
outputs/                 # 模型、日志、图表输出目录
rlhf_lab/
  prepare_data.py        # 偏好数据预处理
  train_reward_model.py  # 训练奖励模型
  train_ppo.py           # PPO 策略对齐
  evaluate_alignment.py  # 生成对齐前后对比案例
  plot_metrics.py        # 绘制实验曲线
```

## 环境准备

建议在 Conda 环境中运行。进入项目目录后先确认 Python 可用：

```powershell
python --version
```

如果 PowerShell 提示找不到 `python`，说明当前终端还没有激活正确环境。请先打开 Anaconda Prompt，或在 PowerShell 中执行你的 Conda 初始化和激活命令，然后回到本目录。

安装依赖：

```powershell
pip install -r requirements.txt
```

如果 Hugging Face 下载模型较慢，可以把配置文件中的模型名改成本地已下载模型路径。

## 默认实验设置

当前配置偏向小显存机器：

- 基座模型：`Qwen/Qwen2.5-0.5B-Instruct`
- RM 训练集：4000 条偏好样本
- RM 验证集：400 条偏好样本
- PPO 提示词：800 条
- 固定评估提示词：`data/eval_prompts.jsonl`

如果 8 GB 显存仍然 OOM，优先降低以下参数：

- `configs/rm_config.json`：`max_length`、`per_device_train_batch_size`
- `configs/ppo_config.json`：`max_prompt_length`、`reward_max_length`、`batch_size`、`mini_batch_size`、`max_new_tokens`
- PPO 阶段也可以把 `reward_device` 从 `cuda` 改成 `cpu`，让奖励模型在 CPU 上打分。

## 运行步骤

### 1. 处理偏好数据

```powershell
python -m rlhf_lab.prepare_data --config configs/data_config.json
```

预期输出：

- `data/processed/rm_train.jsonl`
- `data/processed/rm_eval.jsonl`
- `data/processed/ppo_prompts.jsonl`

这一步会把 HH-RLHF 数据处理成 `(prompt, chosen, rejected)` 三元组，其中 `chosen` 是人类更偏好的回答，`rejected` 是被拒绝的回答。

### 2. 训练奖励模型

```powershell
python -m rlhf_lab.train_reward_model --config configs/rm_config.json
```

预期输出：

- `outputs/rm/checkpoint-*`
- `outputs/rm/final_model/`
- `outputs/rm/rm_log_history.json`
- `outputs/rm/eval_metrics.json`

奖励模型使用的核心目标是让 `chosen` 的分数高于 `rejected`：

```python
loss = -log(sigmoid(reward_chosen - reward_rejected))
```

报告中需要重点记录 RM 在验证集上的 `Accuracy` 曲线。

### 3. PPO 策略对齐

```powershell
python -m rlhf_lab.train_ppo --config configs/ppo_config.json
```

预期输出：

- `outputs/ppo/ppo_metrics.jsonl`
- `outputs/ppo/checkpoint-step-*`
- `outputs/ppo/final_policy/`

PPO 阶段需要观察两个核心指标：

- `mean_reward`：奖励模型给策略输出的平均奖励；
- `objective/kl`：当前策略相对参考模型的偏移程度。

如果 `KL` 过高，说明模型偏离原始语言分布较多，可能出现重复、乱码或奖励作弊；如果奖励长期不上升，可以适当检查奖励模型质量、学习率和采样参数。

### 4. 生成 Case Study

```powershell
python -m rlhf_lab.evaluate_alignment --config configs/eval_config.json
```

预期输出：

- `outputs/eval/alignment_cases.jsonl`
- `outputs/eval/case_study.md`

`data/eval_prompts.jsonl` 中包含有用性、安全性和边界场景提示词。报告至少选 3 组对齐前后回答进行分析，重点说明回答风格、安全性和拒绝不安全指令的能力是否发生变化。

### 5. 绘制报告曲线

PowerShell 中运行：

```powershell
python -m rlhf_lab.plot_metrics `
  --rm-log outputs/rm/rm_log_history.json `
  --ppo-log outputs/ppo/ppo_metrics.jsonl `
  --output-dir outputs/plots
```

预期输出：

- `outputs/plots/rm_loss.png`
- `outputs/plots/rm_accuracy.png`
- `outputs/plots/ppo_reward.png`
- `outputs/plots/ppo_kl.png`

## 配置文件说明

### `configs/data_config.json`

- `dataset_name`：Hugging Face 数据集名称；
- `max_rm_train_samples`：RM 训练样本数；
- `max_rm_eval_samples`：RM 验证样本数；
- `max_ppo_prompts`：PPO 使用的提示词数量。

### `configs/rm_config.json`

- `model_name`：奖励模型基座；
- `max_length`：RM 输入最大长度；
- `learning_rate`：RM 学习率；
- `per_device_train_batch_size`：单卡训练 batch size，小显存建议保持为 `1`；
- `gradient_accumulation_steps`：梯度累积步数，想增大有效 batch size 时优先调这个；
- `use_lora`：是否使用 LoRA 微调。

### `configs/ppo_config.json`

- `model_name`：Actor 和 Reference 的初始化模型；
- `reward_model_path`：训练好的奖励模型路径；
- `target_kl`：PPO 的目标 KL；
- `init_kl_coef`：KL 惩罚初始系数；
- `max_steps`：PPO 优化步数；
- `temperature`、`top_p`、`max_new_tokens`：Rollout 生成参数；
- `reward_device`：奖励模型运行设备，显存不足时可设为 `cpu`。

### `configs/eval_config.json`

- `base_model_name`：PPO 前的基座模型；
- `aligned_model_path`：PPO 后的策略模型；
- `prompts_file`：评估提示词文件。

## 实验报告对应关系

报告要求与本项目输出文件的对应关系如下：

- 实验配置：`configs/*.json`
- RM Accuracy 曲线：`outputs/plots/rm_accuracy.png`
- PPO Mean Reward 曲线：`outputs/plots/ppo_reward.png`
- PPO KL Divergence 曲线：`outputs/plots/ppo_kl.png`
- Case Study：`outputs/eval/case_study.md`
- RM 最终指标：`outputs/rm/eval_metrics.json`
- PPO 原始日志：`outputs/ppo/ppo_metrics.jsonl`

提交要求：

- 提交实验报告 PDF 和核心代码压缩包；
- 邮件发送至 `sdzhao@ir.hit.edu.cn`；
- 邮件主题和压缩包文件名格式：`[实验四]-学号-姓名.zip`。

## 建议实验记录

训练过程中建议记录以下信息，方便写报告：

- RM：`learning_rate`、`batch_size`、`gradient_accumulation_steps`、`max_length`、最终 `eval_accuracy`；
- PPO：`target_kl`、`init_kl_coef`、`temperature`、`top_p`、`max_new_tokens`、最终平均奖励和 KL 趋势；
- Case Study：至少包含 1 个有用性问题、1 个不安全指令、1 个边界场景问题；
- KL 分析：可以额外尝试降低 `init_kl_coef` 或增大 PPO 学习率，观察是否出现 KL 过大、重复输出或奖励异常升高。

## 常见问题

- `python` 无法识别：先激活 Conda 环境，或把 Python 加入 PATH。
- 下载数据或模型失败：确认网络能访问 Hugging Face，或改用本地模型路径和本地数据缓存。
- 显存不足：优先降低长度和 batch size；PPO 阶段可把 `reward_device` 改为 `cpu`。
- RM Accuracy 很低：增加训练样本数或训练轮数，检查 `chosen/rejected` 数据是否正常。
- PPO Reward 上升但回答变差：重点查看 `objective/kl`，这通常表示策略偏移过大或奖励模型被利用。

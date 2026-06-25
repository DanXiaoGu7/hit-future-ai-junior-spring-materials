# 知识图谱补全实验代码

本仓库按实验指南搭建，使用 PyTorch 从零实现 TransE、TransH、TransR，不依赖 OpenKE 等现成知识图谱补全库。`TransE.py`、`TransH.py`、`TransR.py` 是要求提交的模型文件，训练、负采样、数据读取和评估逻辑放在 `kg_completion/` 中复用。

## 目录结构

```text
.
├── FB15k/
├── WN18/
├── TransE.py
├── TransH.py
├── TransR.py
├── kg_completion/
│   ├── cli.py
│   ├── data.py
│   ├── evaluation.py
│   ├── sampling.py
│   └── training.py
├── requirements.txt
└── README.md
```

## 环境安装

建议使用 Python 3.10+。

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install -r requirements.txt
```

如果你已经安装了支持 CUDA 的 PyTorch，可以直接使用现有环境。

## 运行命令

以下命令会训练模型、保存 checkpoint，并在测试集上做关系预测评估，输出 `Hits@10` 和 `Mean Rank`。

### TransE

```bash
python TransE.py --data WN18 --dim 100 --epochs 1000 --batch-size 1024 --lr 0.001 --margin 1.0 --p-norm 1 --save checkpoints/transe_wn18.pt
```
relation_prediction count=5000 hits@10=0.9978 mean_rank=1.80
```bash
python TransE.py --data FB15k --dim 100 --epochs 1000 --batch-size 1024 --lr 0.001 --margin 1.0 --p-norm 1 --save checkpoints/transe_fb15k.pt --n-to-n-eval
```
relation_prediction count=59071 hits@10=0.4449 mean_rank=194.50
n-n_entity_prediction count=44309 head_hits@10=0.0004 head_mean_rank=7552.00 tail_hits@10=0.0007 tail_mean_rank=7484.38
### TransH

```bash
python TransH.py --data WN18 --dim 100 --epochs 1000 --batch-size 1024 --lr 0.001 --margin 1.0 --p-norm 1 --sampling bern --save checkpoints/transh_wn18.pt
```
 relation_prediction count=5000 hits@10=0.9934 mean_rank=2.49
 ```bash
python TransH.py --data FB15k --dim 100 --epochs 1000 --batch-size 1024 --lr 0.001 --margin 1.0 --p-norm 1 --sampling bern --save checkpoints/transh_fb15k.pt --n-to-n-eval
```
relation_prediction count=59071 hits@10=0.3721 mean_rank=243.17
n-n_entity_prediction count=44309 head_hits@10=0.0006 head_mean_rank=7497.60 tail_hits@10=0.0007 tail_mean_rank=7549.34

### TransR

```bash
python TransR.py --data WN18 --ent-dim 100 --rel-dim 100 --epochs 1000 --batch-size 512 --lr 0.001 --margin 1.0 --p-norm 1 --save checkpoints/transr_wn18.pt
```
relation_prediction count=5000 hits@10=0.9662 mean_rank=3.04
```bash
python TransR.py --data FB15k --ent-dim 100 --rel-dim 100 --epochs 1000 --batch-size 512 --lr 0.001 --margin 1.0 --p-norm 1 --save checkpoints/transr_fb15k.pt --n-to-n-eval
```
relation_prediction count=59071 hits@10=0.0000 mean_rank=810.69

当前 `checkpoints/transr_fb15k.pt` 的关系预测结果异常，不建议继续用它做完整 n-to-n。建议用更稳定的参数重新训练一个新 checkpoint：

```bash
python TransR.py --data FB15k --ent-dim 100 --rel-dim 100 --epochs 1000 --batch-size 512 --lr 0.0005 --margin 1.0 --p-norm 1 --sampling bern --reg-weight 0.001 --max-grad-norm 5.0 --save checkpoints/transr_fb15k_v2.pt
```

## 只评估已训练模型

```bash
python TransE.py --data WN18 --load checkpoints/transe_wn18.pt --no-train
python TransH.py --data FB15k --load checkpoints/transh_fb15k.pt --no-train --n-to-n-eval
python TransR.py --data FB15k --load checkpoints/transr_fb15k_v2.pt --no-train --n-to-n-eval
```

## 快速复查 n-to-n 评估

如果只想检查已有 checkpoint 的 FB15k n-to-n 头/尾实体预测，不需要重新训练。先用小样本确认评估逻辑和速度：

```bash
python TransE.py --data FB15k --dim 100 --load checkpoints/transe_fb15k.pt --eval-only --skip-relation-eval --n-to-n-eval --max-n2n-eval 1000
python TransH.py --data FB15k --dim 100 --load checkpoints/transh_fb15k.pt --eval-only --skip-relation-eval --n-to-n-eval --max-n2n-eval 1000
python TransR.py --data FB15k --ent-dim 100 --rel-dim 100 --load checkpoints/transr_fb15k_v2.pt --eval-only --skip-relation-eval --n-to-n-eval --max-n2n-eval 500
```

确认能正常输出后再跑完整 n-to-n：

```bash
python TransE.py --data FB15k --dim 100 --load checkpoints/transe_fb15k.pt --eval-only --skip-relation-eval --n-to-n-eval
python TransH.py --data FB15k --dim 100 --load checkpoints/transh_fb15k.pt --eval-only --skip-relation-eval --n-to-n-eval
python TransR.py --data FB15k --ent-dim 100 --rel-dim 100 --load checkpoints/transr_fb15k_v2.pt --eval-only --skip-relation-eval --n-to-n-eval
```

## 常用参数

- `--filtered`：使用 filtered ranking，评估时会忽略其他已知正确三元组。
- `--sampling unif|bern`：负采样方式。TransH 论文中的 bern 采样可用 `--sampling bern`。
- `--max-grad-norm N`：训练时做梯度裁剪，TransR 在 FB15k 上建议使用 `5.0`。
- `--n-to-n-eval`：读取 `n-n.txt`，评估 FB15k 中 n-to-n 关系下的头实体和尾实体 `Hits@10`。
- `--max-n2n-eval N`：只评估前 N 条 n-to-n 样本，适合 debug。
- `--entity-progress-every N`：n-to-n 实体预测每处理 N 条样本输出一次进度。
- `--type-split 1-1|1-n|n-1|n-n`：切换实体预测评估的关系类型文件。
- `--device cpu|cuda|cuda:0|auto`：默认 `auto`，优先使用 CUDA。
- `--candidate-chunk-size` / `--candidate-batch-size`：实体预测评估会枚举全部实体，显存不足时调小该值。
- `--eval-only`：加载 checkpoint 后直接评估，不再训练。

## 输出指标说明

关系预测输出示例：

```text
relation_prediction count=5000 hits@10=0.7500 mean_rank=263.00
```

n-to-n 实体预测输出示例：

```text
n-n_entity_prediction count=44309 head_hits@10=0.3200 head_mean_rank=200.00 tail_hits@10=0.4100 tail_mean_rank=180.00
```

实验报告中可分别记录 WN18、FB15k 的关系预测 `Hits@10`、`Mean Rank`，以及 FB15k 上 n-to-n 关系的头实体和尾实体 `Hits@10`，再对比 TransE、TransH、TransR。

## 数据说明

代码会自动读取 `train.txt`、`valid.txt`、`test.txt`。如果训练集只有 `train.zip`，会自动读取压缩包内的 `train.txt`，不需要手动解压。

# LAB2: Hopfield on MNIST

实验二使用 `Hopfield` 联想记忆网络处理 `MNIST` 手写数字分类任务。

## 项目结构

```text
LAB2
├── train.py
├── eval.py
├── requirements.txt
├── scripts
└── src
```

## 安装依赖

```powershell
pip install -r requirements.txt
```

## 训练

```powershell
python train.py --data_path ./data --ckpt_path ./ckpt
```

## 测试

```powershell
python eval.py --data_path ./data --ckpt_path ./ckpt/hopfield_mnist.pt
```

## 输出文件

- `ckpt/hopfield_mnist.pt`：Hopfield 模型权重和记忆模式
- `ckpt/summary.json`：训练后保存的结果摘要

## 说明

- 默认使用多个类别记忆模式来表示每个数字类别
- 默认通过“最近记忆模式匹配”完成分类
- 如需尝试迭代召回，可额外传入 `--recall_steps`

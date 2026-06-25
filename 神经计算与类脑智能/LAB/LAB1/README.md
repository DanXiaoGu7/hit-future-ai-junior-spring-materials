# LAB1: MLP on MNIST

实验一使用 `PyTorch` 搭建多层感知器（MLP），完成 `MNIST` 手写数字分类。

## 项目结构

```text
LAB1
├── train.py
├── eval.py
├── plot_metrics.py
├── requirements.txt
├── ckpt
├── compare_one_hidden
├── data
├── figures
├── scripts
└── src
```

其中 `compare_one_hidden` 为实验一的一层隐藏层对比版本。

## 安装依赖

先进入 `LAB1` 目录：

```powershell
cd D:\A2026CSLAB\Neural_computation\LAB1
```

再安装依赖：

```powershell
pip install -r requirements.txt
```

## 训练

在 `LAB1` 目录下运行：

```powershell
python train.py --data_path ./data --ckpt_path ./ckpt
```

## 测试

在 `LAB1` 目录下运行：

```powershell
python eval.py --data_path ./data --ckpt_path ./ckpt/best_mlp_mnist.pt
```

## 绘图

```powershell
python plot_metrics.py
```

## 输出文件

- `ckpt/best_mlp_mnist.pt`：最佳模型参数
- `ckpt/history.json`：每轮训练和测试结果
- `figures/loss_curve.png`：损失曲线
- `figures/accuracy_curve.png`：准确率曲线
- `figures/metrics_curve.png`：综合曲线

## 对比版本

一层隐藏层对比实验位于 `LAB1/compare_one_hidden`。

# LAB1 Compare: One Hidden Layer MLP

这是实验一的对比版本，只保留一层隐藏层，现已并入 `LAB1/compare_one_hidden`。

## 安装依赖

```powershell
pip install -r requirements.txt
```

## 训练

建议直接复用 `LAB1` 已下载的数据：

```powershell
python train.py --data_path ../data --ckpt_path ./ckpt
```

## 测试

```powershell
python eval.py --data_path ../data --ckpt_path ./ckpt/best_mlp_one_hidden_mnist.pt
```

## 输出文件

- `ckpt/best_mlp_one_hidden_mnist.pt`：对比版模型参数
- `ckpt/history.json`：每轮训练和测试结果

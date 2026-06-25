下面是 outputs 文件夹中每个文件的作用整理：

mnist_stats.json：MNIST 训练集像素的均值、方差、标准差（用于归一化说明与截图）
samples_per_class.png：每个类别随机展示 ≥5 个已归一化样本
model_params.json：模型 A / B 的总参数量与可训练参数量
模型 A

modelA_train_log.csv：训练过程逐步记录（step、loss、acc），用于画训练曲线
modelA_train_curves.png：训练损失曲线 + 准确率曲线（x轴为 step）
modelA_epoch_log.csv：每个 epoch 的训练损失、训练准确率、测试准确率、耗时
modelA_confusion.png：模型 A 测试集混淆矩阵
modelA_metrics.json：模型 A 指标（accuracy、precision/recall/f1 的 macro 与 weighted、每类指标）
modelA_wrong_samples.png：模型 A 错误分类样本图（≥5）
modelA_weights.pth：模型 A 训练后的参数权重
模型 B

modelB_train_log.csv：训练过程逐步记录（step、loss、acc），用于画训练曲线
modelB_train_curves.png：训练损失曲线 + 准确率曲线（x轴为 step）
modelB_epoch_log.csv：每个 epoch 的训练损失、训练准确率、测试准确率、耗时
modelB_confusion.png：模型 B 测试集混淆矩阵
modelB_metrics.json：模型 B 指标（accuracy、precision/recall/f1 的 macro 与 weighted、每类指标）
modelB_wrong_samples.png：模型 B 错误分类样本图（≥5）
modelB_weights.pth：模型 B 训练后的参数权重
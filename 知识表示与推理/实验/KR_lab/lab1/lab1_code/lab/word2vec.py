from gensim.models import word2vec

# 加载语料
sentences = word2vec.Text8Corpus('./data/seg_text.txt')

# 训练模型
window = 1 # 窗口大小
vector_size = 64  # 嵌入向量维度
sg = 1  # 是否使用skip-gram 0表示否
epochs = 3  # 训练轮次
seed = 42  # 随机种子

# min_count=1 保留低频词，确保“蓝银草”等新词进入词表
model = word2vec.Word2Vec(
    sentences,
    window=window,
    vector_size=vector_size,
    epochs=epochs,
    seed=seed,
    sg=sg,
    min_count=1,
)

model.save("./ckpt/1019.model")

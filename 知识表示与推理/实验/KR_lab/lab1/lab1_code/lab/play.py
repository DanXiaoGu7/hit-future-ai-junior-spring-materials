from gensim.models import word2vec
import matplotlib

matplotlib.rc("font", family="YouYuan")  # 确保中文可显示

# 载入训练好的模型
model = word2vec.Word2Vec.load("./ckpt/1019.model")


def print_similarity(a: str, b: str):
    """打印两个词的相似度；若词不存在则给出提示。"""
    missing = [w for w in (a, b) if w not in model.wv]
    if missing:
        print(f"词 {missing} 不在词表中，请确保语料包含这些词后重新训练。")
        return
    sim = model.wv.similarity(a, b)
    print(f"{a} 和 {b} 的相似度: {sim:.4f}")


if __name__ == "__main__":
    # 两对相似词
    print("——相似——")
    print_similarity("唐三", "唐昊")
    print_similarity("唐三", "小舞")

    # 两对不相似词
    print("\n——不相似——")
    print_similarity("唐昊", "蓝银草")
    print_similarity("海神", "蓝银草")

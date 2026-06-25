from ltp import LTP


def run_demo():
    ltp = LTP("./ltp_small")
    sentences = [
        "小明同学于今年暑假游览武汉。",
        "裴友生，男，汉族，湖北蕲春人，1957年12月出生，大专学历。",
        "《民航客运服务会话》是1995年中国民航出版社出版的图书，作者是周石田。",
    ]
    result = ltp.pipeline(sentences, tasks=["cws", "ner"])
    for sentence, ner in zip(sentences, result.ner):
        print(sentence)
        print(ner)


if __name__ == "__main__":
    run_demo()

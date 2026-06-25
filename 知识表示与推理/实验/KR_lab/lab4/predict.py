import json

import torch
from transformers import BertTokenizer

from config import NerConfig
from model import BertNer


def build_inputs(sentence, tokenizer, max_seq_len):
    text = list(sentence)
    if len(text) > max_seq_len - 2:
        text = text[: max_seq_len - 2]

    input_ids = tokenizer.convert_tokens_to_ids(["[CLS]"] + text + ["[SEP]"])
    attention_mask = [1] * len(input_ids)
    pad_len = max_seq_len - len(input_ids)

    input_ids = input_ids + [0] * pad_len
    attention_mask = attention_mask + [0] * pad_len

    return (
        text,
        torch.tensor([input_ids], dtype=torch.long),
        torch.tensor([attention_mask], dtype=torch.long),
    )


def decode_entities(text, labels):
    entities = []
    start = None
    ent_type = None

    for idx, label in enumerate(labels + ["O"]):
        if label.startswith("B-"):
            if start is not None:
                entities.append(
                    {
                        "text": "".join(text[start:idx]),
                        "label": ent_type,
                        "start": start,
                        "end": idx,
                    }
                )
            start = idx
            ent_type = label[2:]
        elif label.startswith("I-") and ent_type == label[2:]:
            continue
        else:
            if start is not None:
                entities.append(
                    {
                        "text": "".join(text[start:idx]),
                        "label": ent_type,
                        "start": start,
                        "end": idx,
                    }
                )
                start = None
                ent_type = None

    return entities


def predict(sentences, data_name="duie"):
    args = NerConfig(data_name)
    tokenizer = BertTokenizer.from_pretrained(args.bert_dir)
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

    model = BertNer(args).to(device)
    model.load_state_dict(torch.load(f"{args.output_dir}/pytorch_model_ner.bin", map_location=device))
    model.eval()

    results = []
    with torch.no_grad():
        for sentence in sentences:
            text, input_ids, attention_mask = build_inputs(sentence, tokenizer, args.max_seq_len)
            input_ids = input_ids.to(device)
            attention_mask = attention_mask.to(device)

            output = model(input_ids, attention_mask)
            pred_ids = output.logits[0][1 : len(text) + 1]
            pred_labels = [args.id2label[idx] for idx in pred_ids]
            entities = decode_entities(text, pred_labels)
            results.append({"text": sentence, "entities": entities})

    return results


if __name__ == "__main__":
    demo_sentences = [
        "《民航客运服务会话》是1995年中国民航出版社出版的图书，作者是周石田",
        "再有之后的《半生缘》，蒋勤勤饰演的顾曼璐完全把林心如的曼桢衬得像是涉世未深的小姑娘，毫无半点风情",
        "裴友生，男，汉族，湖北蕲春人，1957年12月出生，大专学历",
        "吴君如演的周吉是电影《花田喜事》，在周吉大婚之夜，其夫林嘉声逃走失踪，后来其夫新科状元高中回来，周吉急往城楼相识，但林嘉声却言夫妻情断，覆水难收",
    ]

    print(json.dumps(predict(demo_sentences), ensure_ascii=False, indent=2))

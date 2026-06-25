from transformers import AutoTokenizer, AutoModelForTokenClassification
from datasets import load_dataset
from transformers import TrainingArguments, Trainer
from transformers import DataCollatorForTokenClassification
import evaluate
import numpy as np
from datasets import DownloadMode
label = ['O', 'B-PER', 'I-PER', 'B-ORG', 'I-ORG', 'B-LOC', 'I-LOC', 'B-MISC', 'I-MISC']
id2label, label2id = {}, {}
for idx, item in enumerate(label):
    id2label[idx] = item
    label2id[item] = idx
############模型定义
tokenizer = AutoTokenizer.from_pretrained("./cache/distilbert")
model = AutoModelForTokenClassification.from_pretrained(
    "./cache/distilbert", num_labels=9, id2label=id2label, label2id=label2id
)

###########数据集准备

dataset = load_dataset("conll2003", cache_dir="./cache",download_mode=DownloadMode.REUSE_DATASET_IF_EXISTS)
label_list = dataset["train"].features[f"ner_tags"].feature.names

def tokenize_and_align_labels(examples):
    tokenized_inputs = tokenizer(examples["tokens"], truncation=True, is_split_into_words=True)

    labels = []
    for i, label in enumerate(examples[f"ner_tags"]):
        word_ids = tokenized_inputs.word_ids(batch_index=i)  # Map tokens to their respective word.
        previous_word_idx = None
        label_ids = []
        for word_idx in word_ids:  # Set the special tokens to -100.
            if word_idx is None:
                label_ids.append(-100)
            elif word_idx != previous_word_idx:  # Only label the first token of a given word.
                label_ids.append(label[word_idx])
            else:
                label_ids.append(-100)
            previous_word_idx = word_idx
        labels.append(label_ids)

    tokenized_inputs["labels"] = labels
    return tokenized_inputs

tokenized_dataset = dataset.map(tokenize_and_align_labels, batched=True)
data_collator = DataCollatorForTokenClassification(tokenizer=tokenizer)

##########评测指标
seqeval = evaluate.load("seqeval")
example = dataset["train"][0]
labels = [label_list[i] for i in example[f"ner_tags"]]

def compute_metrics(p):
    predictions, labels = p
    predictions = np.argmax(predictions, axis=2)

    true_predictions = [
        [label_list[p] for (p, l) in zip(prediction, label) if l != -100]
        for prediction, label in zip(predictions, labels)
    ]
    true_labels = [
        [label_list[l] for (p, l) in zip(prediction, label) if l != -100]
        for prediction, label in zip(predictions, labels)
    ]

    results = seqeval.compute(predictions=true_predictions, references=true_labels)
    return {
        "precision": results["overall_precision"],
        "recall": results["overall_recall"],
        "f1": results["overall_f1"],
        "accuracy": results["overall_accuracy"],
    }

###########训练参数
training_args = TrainingArguments(
    output_dir="./ckpt/NER_ckpt",
    learning_rate=2e-5,
    per_device_train_batch_size=8,
    per_device_eval_batch_size=8,
    num_train_epochs=1,
    evaluation_strategy="epoch",
    weight_decay=0.01,
    save_strategy="epoch",
)

###############模型训练
trainer = Trainer(
    model=model,
    args=training_args,
    train_dataset=tokenized_dataset["train"],
    eval_dataset=tokenized_dataset["test"],
    tokenizer=tokenizer,
    data_collator=data_collator,
    compute_metrics=compute_metrics,
)

trainer.train()
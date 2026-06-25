# -*- coding: utf-8 -*-
import os
import re
import json

from tqdm import tqdm
from collections import defaultdict


class ProcessDuieData:
    def __init__(self):
        base_dir = os.path.dirname(os.path.abspath(__file__))
        raw_data_dir = os.path.join(base_dir, "duie_data")

        self.data_path = os.path.join(base_dir, "data", "duie") + os.sep
        self.train_file = os.path.join(raw_data_dir, "train.json")
        self.dev_file = os.path.join(raw_data_dir, "dev.json")
        self.test_file = os.path.join(raw_data_dir, "test.json")
        self.schema_file = os.path.join(raw_data_dir, "duie_schema.json")

        os.makedirs(os.path.join(self.data_path, "ner_data"), exist_ok=True)
        os.makedirs(os.path.join(self.data_path, "re_data"), exist_ok=True)

    @staticmethod
    def _normalize_ent_type(ent_type):
        return "人物" if "人物" in ent_type else ent_type

    @staticmethod
    def _find_all(entity, text):
        if not entity:
            return []
        return list(re.finditer(re.escape(entity), text))

    def get_ents(self):
        ents = set()
        rels = defaultdict(list)
        with open(self.schema_file, "r", encoding="utf-8") as fp:
            lines = fp.readlines()
            for line in lines:
                data = json.loads(line)
                subject_type = self._normalize_ent_type(data["subject_type"])
                object_type = self._normalize_ent_type(data["object_type"]["@value"])
                ents.add(subject_type)
                ents.add(object_type)
                predicate = data["predicate"]
                rels[subject_type + "_" + object_type].append(predicate)

        with open(self.data_path + "ner_data/labels.txt", "w", encoding="utf-8") as fp:
            fp.write("\n".join(sorted(list(ents))))

        with open(self.data_path + "re_data/rels.txt", "w", encoding="utf-8") as fp:
            json.dump(rels, fp, ensure_ascii=False, indent=2)

    def get_ner_data(self, input_file, output_file):
        res = []
        with open(input_file, "r", encoding="utf-8", errors="replace") as fp:
            lines = fp.read().strip().split("\n")
            for i, line in enumerate(tqdm(lines)):
                try:
                    line = json.loads(line)
                except Exception:
                    continue
                tmp = {}
                text = line["text"]
                tmp["text"] = [i for i in text]
                tmp["labels"] = ["O"] * len(text)
                tmp["id"] = i
                spo_list = line["spo_list"]
                for spo in spo_list:
                    if spo["subject"] == "" or spo["object"]["@value"] == "":
                        continue
                    subject_re_res = self._find_all(spo["subject"], line["text"])
                    subject_type = self._normalize_ent_type(spo["subject_type"])
                    for sbj in subject_re_res:
                        sbj_start, sbj_end = sbj.span()
                        tmp["labels"][sbj_start] = f"B-{subject_type}"
                        for j in range(sbj_start + 1, sbj_end):
                            tmp["labels"][j] = f"I-{subject_type}"

                    object_re_res = self._find_all(spo["object"]["@value"], line["text"])
                    object_type = self._normalize_ent_type(spo["object_type"]["@value"])
                    for obj in object_re_res:
                        obj_start, obj_end = obj.span()
                        tmp["labels"][obj_start] = f"B-{object_type}"
                        for j in range(obj_start + 1, obj_end):
                            tmp["labels"][j] = f"I-{object_type}"
                res.append(tmp)

        with open(output_file, "w", encoding="utf-8") as fp:
            fp.write("\n".join([json.dumps(i, ensure_ascii=False) for i in res]))


if __name__ == "__main__":
    processDuieData = ProcessDuieData()
    processDuieData.get_ents()
    processDuieData.get_ner_data(
        processDuieData.train_file,
        os.path.join(processDuieData.data_path, "ner_data", "train.txt"),
    )
    processDuieData.get_ner_data(
        processDuieData.dev_file,
        os.path.join(processDuieData.data_path, "ner_data", "dev.txt"),
    )

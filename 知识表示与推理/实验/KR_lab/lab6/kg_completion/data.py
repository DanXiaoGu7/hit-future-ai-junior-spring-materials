from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Sequence, Tuple
import zipfile

Triple = Tuple[int, int, int]


@dataclass
class KnowledgeGraphData:
    """Container for mappings and train/valid/test triples."""

    data_dir: Path
    entity2id: Dict[str, int]
    relation2id: Dict[str, int]
    id2entity: List[str]
    id2relation: List[str]
    train_triples: List[Triple]
    valid_triples: List[Triple]
    test_triples: List[Triple]

    @property
    def ent_num(self) -> int:
        return len(self.entity2id)

    @property
    def rel_num(self) -> int:
        return len(self.relation2id)

    @property
    def all_true_triples(self) -> set[Triple]:
        return set(self.train_triples) | set(self.valid_triples) | set(self.test_triples)

    @classmethod
    def load(cls, data_dir: str | Path) -> "KnowledgeGraphData":
        data_path = Path(data_dir)
        entity2id = read_mapping(data_path / "entity2id.txt")
        relation2id = read_mapping(data_path / "relation2id.txt")
        id2entity = invert_mapping(entity2id)
        id2relation = invert_mapping(relation2id)

        return cls(
            data_dir=data_path,
            entity2id=entity2id,
            relation2id=relation2id,
            id2entity=id2entity,
            id2relation=id2relation,
            train_triples=read_triples(data_path, "train", entity2id, relation2id),
            valid_triples=read_triples(data_path, "valid", entity2id, relation2id),
            test_triples=read_triples(data_path, "test", entity2id, relation2id),
        )

    def read_type_triples(self, type_name: str) -> List[Triple]:
        """Read 1-1, 1-n, n-1, or n-n files, whose triples are already IDs."""

        path = self.data_dir / f"{type_name}.txt"
        if not path.exists():
            raise FileNotFoundError(f"cannot find split file: {path}")
        triples: List[Triple] = []
        with path.open("r", encoding="utf-8") as f:
            for line in f:
                parts = line.strip().split()
                if len(parts) == 1:
                    continue
                if len(parts) < 3:
                    continue
                h, t, r = map(int, parts[:3])
                triples.append((h, t, r))
        return triples


def read_mapping(path: Path) -> Dict[str, int]:
    mapping: Dict[str, int] = {}
    with path.open("r", encoding="utf-8") as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) < 2:
                continue
            mapping[parts[0]] = int(parts[1])
    return mapping


def invert_mapping(mapping: Dict[str, int]) -> List[str]:
    values = [""] * len(mapping)
    for key, idx in mapping.items():
        values[idx] = key
    return values


def read_triples(
    data_dir: Path,
    split: str,
    entity2id: Dict[str, int],
    relation2id: Dict[str, int],
) -> List[Triple]:
    triples: List[Triple] = []
    for line in iter_split_lines(data_dir, split):
        parts = line.strip().split()
        if len(parts) < 3:
            continue
        h, t, r = parts[:3]
        triples.append(parse_triple(h, t, r, entity2id, relation2id))
    return triples


def iter_split_lines(data_dir: Path, split: str) -> Iterable[str]:
    txt_path = data_dir / f"{split}.txt"
    if txt_path.exists():
        with txt_path.open("r", encoding="utf-8") as f:
            yield from f
        return

    zip_path = data_dir / f"{split}.zip"
    if zip_path.exists():
        inner_name = f"{split}.txt"
        with zipfile.ZipFile(zip_path) as zf:
            with zf.open(inner_name) as raw:
                for line in raw:
                    yield line.decode("utf-8")
        return

    raise FileNotFoundError(f"cannot find {split}.txt or {split}.zip in {data_dir}")


def parse_triple(
    h: str,
    t: str,
    r: str,
    entity2id: Dict[str, int],
    relation2id: Dict[str, int],
) -> Triple:
    if h.isdigit() and t.isdigit() and r.isdigit():
        return int(h), int(t), int(r)
    return entity2id[h], entity2id[t], relation2id[r]


def batch_iter(items: Sequence[Triple], batch_size: int) -> Iterable[List[Triple]]:
    for start in range(0, len(items), batch_size):
        yield list(items[start : start + batch_size])


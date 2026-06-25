from __future__ import annotations

import argparse
from pathlib import Path
from typing import Type

import torch
from torch import nn

from .data import KnowledgeGraphData
from .evaluation import evaluate_entity_prediction, evaluate_relation_prediction
from .training import TrainConfig, load_checkpoint, save_checkpoint, set_seed, train_model


def run_model_cli(model_name: str, model_class: Type[nn.Module]) -> None:
    parser = build_parser(model_name)
    args = parser.parse_args()
    set_seed(args.seed)

    device = choose_device(args.device)
    kg = KnowledgeGraphData.load(args.data)
    model = build_model(model_name, model_class, kg.ent_num, kg.rel_num, args).to(device)

    if args.eval_only:
        args.no_train = True

    if args.no_train and not args.load:
        raise ValueError("--no-train/--eval-only requires --load so that evaluation does not use random weights.")

    if args.load:
        load_checkpoint(model, args.load, device)

    if not args.no_train:
        train_config = TrainConfig(
            epochs=args.epochs,
            batch_size=args.batch_size,
            lr=args.lr,
            margin=args.margin,
            reg_weight=args.reg_weight,
            negative_size=args.negative_size,
            sampling=args.sampling,
            max_grad_norm=args.max_grad_norm,
            num_workers=args.num_workers,
            log_every=args.log_every,
            seed=args.seed,
            device=device,
        )
        train_model(model, kg.train_triples, kg.all_true_triples, kg.ent_num, train_config)
        if args.save:
            save_checkpoint(model, args.save)

    if not args.skip_eval:
        metrics = evaluate_relation_prediction(
            model=model,
            triples=kg.test_triples,
            rel_num=kg.rel_num,
            device=device,
            batch_size=args.eval_batch_size,
            filtered=args.filtered,
            all_true_triples=kg.all_true_triples,
        )
        print(
            f"relation_prediction count={metrics.count} "
            f"hits@10={metrics.hits_at_10:.4f} mean_rank={metrics.mean_rank:.2f}"
        )

    if args.n_to_n_eval:
        triples = kg.read_type_triples(args.type_split)
        entity_metrics = evaluate_entity_prediction(
            model=model,
            triples=triples,
            ent_num=kg.ent_num,
            device=device,
            batch_size=args.entity_eval_batch_size,
            candidate_chunk_size=args.candidate_chunk_size,
            filtered=args.filtered,
            all_true_triples=kg.all_true_triples,
            max_eval=args.max_n2n_eval,
            progress_every=args.entity_progress_every,
        )
        print(
            f"{args.type_split}_entity_prediction count={entity_metrics.count} "
            f"head_hits@10={entity_metrics.head_hits_at_10:.4f} "
            f"head_mean_rank={entity_metrics.head_mean_rank:.2f} "
            f"tail_hits@10={entity_metrics.tail_hits_at_10:.4f} "
            f"tail_mean_rank={entity_metrics.tail_mean_rank:.2f}"
        )


def build_parser(model_name: str) -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=f"Train and evaluate {model_name.upper()}.")
    parser.add_argument("--data", type=str, required=True, help="Dataset directory, e.g. WN18 or FB15k.")
    parser.add_argument("--dim", type=int, default=100, help="Embedding dimension for TransE/TransH.")
    parser.add_argument("--ent-dim", type=int, default=100, help="Entity dimension for TransR.")
    parser.add_argument("--rel-dim", type=int, default=100, help="Relation dimension for TransR.")
    parser.add_argument("--epochs", type=int, default=1000)
    parser.add_argument("--batch-size", type=int, default=1024)
    parser.add_argument("--lr", type=float, default=0.001)
    parser.add_argument("--margin", type=float, default=1.0)
    parser.add_argument("--p-norm", type=int, choices=[1, 2], default=1)
    parser.add_argument("--reg-weight", type=float, default=1e-5)
    parser.add_argument("--negative-size", type=int, default=1)
    parser.add_argument("--sampling", choices=["unif", "bern"], default="unif")
    parser.add_argument("--max-grad-norm", type=float, default=0.0, help="Clip gradients when positive; useful for TransR.")
    parser.add_argument("--seed", type=int, default=2026)
    parser.add_argument("--device", type=str, default="auto", help="auto, cpu, cuda, or cuda:0.")
    parser.add_argument("--num-workers", type=int, default=0)
    parser.add_argument("--log-every", type=int, default=10)
    parser.add_argument("--eval-batch-size", type=int, default=128)
    parser.add_argument("--entity-eval-batch-size", type=int, default=16)
    parser.add_argument(
        "--candidate-chunk-size",
        "--candidate-batch-size",
        dest="candidate_chunk_size",
        type=int,
        default=2048,
        help="Number of entity candidates scored per chunk during head/tail evaluation.",
    )
    parser.add_argument("--max-n2n-eval", type=int, default=None, help="Only evaluate the first N triples from the type split.")
    parser.add_argument("--entity-progress-every", type=int, default=500, help="Print n-to-n entity evaluation progress every N triples.")
    parser.add_argument("--filtered", action="store_true", help="Use filtered ranking metrics.")
    parser.add_argument("--n-to-n-eval", action="store_true", help="Evaluate head/tail entity prediction on a type split.")
    parser.add_argument("--type-split", choices=["1-1", "1-n", "n-1", "n-n"], default="n-n")
    parser.add_argument("--no-train", action="store_true", help="Only evaluate a loaded checkpoint.")
    parser.add_argument("--eval-only", action="store_true", help="Alias for --no-train.")
    parser.add_argument("--skip-eval", "--skip-relation-eval", dest="skip_eval", action="store_true", help="Skip relation prediction evaluation.")
    parser.add_argument("--load", type=str, default=None, help="Checkpoint to load before training/evaluation.")
    parser.add_argument("--save", type=str, default=default_checkpoint(model_name), help="Checkpoint output path.")
    return parser


def default_checkpoint(model_name: str) -> str:
    return str(Path("checkpoints") / f"{model_name}.pt")


def choose_device(device_arg: str) -> torch.device:
    if device_arg == "auto":
        return torch.device("cuda" if torch.cuda.is_available() else "cpu")
    return torch.device(device_arg)


def build_model(
    model_name: str,
    model_class: Type[nn.Module],
    ent_num: int,
    rel_num: int,
    args: argparse.Namespace,
) -> nn.Module:
    if model_name == "transr":
        return model_class(
            ent_num=ent_num,
            rel_num=rel_num,
            ent_dim=args.ent_dim,
            rel_dim=args.rel_dim,
            margin=args.margin,
            p_norm=args.p_norm,
        )
    return model_class(
        ent_num=ent_num,
        rel_num=rel_num,
        dim=args.dim,
        margin=args.margin,
        p_norm=args.p_norm,
    )

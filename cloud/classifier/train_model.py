"""Train and evaluate the handout-aligned PyTorch temporal model on CPU."""
import argparse
import json
import os
from pathlib import Path
import platform
import sys
import time
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

import numpy as np
import torch

from cloud.classifier.temporal_model import (
    ActivityGRU, FORMAT, HIDDEN_SIZE, LABELS, SENSOR_COLUMNS, WINDOW_SIZE,
    rotate_windows, sha256,
)
from cloud.classifier.temporal_data import build_windows, day_folds, read_dataset


def metrics(actual, predicted):
    matrix = np.zeros((2, 2), dtype=int)
    for a, p in zip(actual, predicted):
        matrix[int(a), int(p)] += 1
    recall = np.divide(np.diag(matrix), matrix.sum(axis=1),
                       out=np.zeros(2), where=matrix.sum(axis=1) != 0)
    return {
        "windows": int(matrix.sum()), "correct": int(np.trace(matrix)),
        "accuracy": float(np.trace(matrix)/matrix.sum()),
        "balanced_accuracy": float(recall.mean()),
        "confusion_matrix_actual_rows_predicted_columns": matrix.tolist(),
        "class_recall": {LABELS[i]: float(recall[i]) for i in range(2)},
    }


def fit(raw, labels, *, epochs, seed):
    torch.manual_seed(seed)
    model = ActivityGRU()
    x = torch.from_numpy(np.asarray(raw, dtype=np.float32))
    y = torch.from_numpy(np.asarray(labels, dtype=np.int64))
    with torch.no_grad():
        normalizer_values = model.features(torch.cat((x, rotate_windows(x))))
        model.mean.copy_(normalizer_values.mean(dim=(0, 1)))
        model.scale.copy_(normalizer_values.std(dim=(0, 1), unbiased=False).clamp_min(1e-5))
    counts = torch.bincount(y, minlength=2).float()
    loss_function = torch.nn.CrossEntropyLoss(weight=len(y)/(2*counts))
    optimizer = torch.optim.AdamW(model.parameters(), lr=0.01, weight_decay=0.001)
    for epoch in range(epochs):
        model.train()
        order = torch.randperm(len(x))
        loss_sum = 0.0
        for indices in order.split(128):
            optimizer.zero_grad(set_to_none=True)
            logits = model(rotate_windows(x[indices]))
            loss = loss_function(logits, y[indices])
            if not torch.isfinite(loss):
                raise RuntimeError("Training loss is not finite.")
            loss.backward()
            torch.nn.utils.clip_grad_norm_(model.parameters(), 1.0)
            optimizer.step()
            loss_sum += float(loss.detach()) * len(indices)
        if (epoch+1) % 20 == 0 or epoch == epochs-1:
            print(f"  epoch {epoch+1}/{epochs}, training loss {loss_sum/len(x):.5f}", flush=True)
    model.eval()
    return model


def predict(model, raw):
    with torch.inference_mode():
        return torch.cat([
            model(batch).argmax(dim=1)
            for batch in torch.from_numpy(raw.astype(np.float32)).split(512)
        ]).numpy()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dataset", type=Path, default=ROOT/"dataset/dataset.csv")
    parser.add_argument("--epochs", type=int, default=60)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--output", type=Path, default=Path(__file__).with_name("activity_model.pt"))
    parser.add_argument("--report", type=Path, default=ROOT/"docs/testing/pytorch_training_2026-10-26.json")
    args = parser.parse_args()
    if not 1 <= args.epochs <= 1000:
        parser.error("epochs must be between 1 and 1000")
    torch.set_num_threads(1)
    torch.use_deterministic_algorithms(True)
    started = time.monotonic()
    frame, dataset_hash = read_dataset(args.dataset)
    x, y, groups, windows, preparation = build_windows(frame)
    days = sorted(np.unique(groups).tolist())
    if len(days) < 7:
        raise ValueError("Need at least seven recording days for development and a two-day holdout.")
    # Fixed before any model scores are inspected: latest two calendar days.
    holdout_days = days[-2:]
    holdout = np.flatnonzero(np.isin(groups, holdout_days))
    development = np.flatnonzero(~np.isin(groups, holdout_days))
    if len(np.unique(y[holdout])) != 2:
        raise ValueError("The untouched date holdout must contain both activities.")
    print(f"Prepared {len(x)} windows, {len(days)} days; holdout days: {holdout_days}", flush=True)
    report = {
        "format": FORMAT, "created_at_utc": datetime.now(timezone.utc).isoformat(),
        "dataset": {"source": "https://www.kaggle.com/datasets/vmalyi/run-or-walk",
                    "creator": "Viktor Malyi", "version": 2, "csv_sha256": dataset_hash,
                    "license": "CC BY-NC-SA 4.0", "users": frame["username"].nunique()},
        "preparation": preparation, "seed": args.seed, "epochs_fixed_before_evaluation": args.epochs,
        "environment": {"python": platform.python_version(), "torch": torch.__version__,
                        "numpy": np.__version__, "cpu_threads": 1},
        "protocol": {
            "input_axes": list(SENSOR_COLUMNS), "window_size": WINDOW_SIZE,
            "nominal_sample_hz": 1.0, "non_overlapping": True,
            "split_unit": "participant/recording date",
            "holdout_days_fixed_before_scoring": holdout_days,
            "development_days": days[:-2],
            "augmentation": "same proper 3D rotation on all samples and both 3D sensors per training window",
            "normalization": "fitted only on the training partition of each evaluation",
            "test_tuning": False,
        },
        "cross_validation": [],
    }
    actual, predicted = [], []
    for fold, (train_local, test_local) in enumerate(
            day_folds(y[development], groups[development]), 1):
        train, test = development[train_local], development[test_local]
        print(f"Development fold {fold}/5: {len(train)} train, {len(test)} test windows", flush=True)
        model = fit(x[train], y[train], epochs=args.epochs, seed=args.seed+fold)
        estimates = predict(model, x[test])
        result = metrics(y[test], estimates)
        result.update({"fold": fold, "train_days": sorted(set(groups[train])),
                       "test_days": sorted(set(groups[test])),
                       "train_windows": len(train)})
        report["cross_validation"].append(result)
        actual.extend(y[test].tolist()); predicted.extend(estimates.tolist())
        print(f"  fold accuracy: {result['accuracy']:.2%}", flush=True)
    report["development_out_of_fold"] = metrics(actual, predicted)
    print("Training on development days; evaluating untouched latest-two-day holdout once.", flush=True)
    model = fit(x[development], y[development], epochs=args.epochs, seed=args.seed+100)
    report["holdout"] = metrics(y[holdout], predict(model, x[holdout]))
    report["holdout"]["days"] = holdout_days
    print(f"Untouched holdout accuracy: {report['holdout']['accuracy']:.2%}", flush=True)
    print("Training deployment model on all available recording days.", flush=True)
    model = fit(x, y, epochs=args.epochs, seed=args.seed+200)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    pending = args.output.with_suffix(".pending")
    with pending.open("wb") as stream:
        torch.save(model.state_dict(), stream)
    os.replace(pending, args.output)
    metadata = {
        "format": FORMAT, "framework": "PyTorch", "torch_version": torch.__version__,
        "architecture": "GRU(8 inputs, 8 hidden) + Linear(8, 2)",
        "hidden_size": HIDDEN_SIZE, "trainable_parameters": sum(p.numel() for p in model.parameters()),
        "sensor_columns": list(SENSOR_COLUMNS), "window_size": WINDOW_SIZE,
        "labels": {str(k): v for k, v in LABELS.items()},
        "units": {"acceleration": "g", "gyroscope": "rad/s"},
        "nominal_sample_hz": 1.0, "dataset_csv_sha256": dataset_hash,
        "model_sha256": sha256(args.output),
        "training_scope": "all 12 recording days; evaluation scores were obtained before this refit",
        "live_board_validation": "pending; prior Random Forest live results do not validate this model",
    }
    args.output.with_suffix(".json").write_text(json.dumps(metadata, indent=2)+"\n", encoding="utf-8")
    report["deployment_model"] = metadata
    report["window_source_rows_sha256"] = __import__("hashlib").sha256(
        json.dumps(windows, sort_keys=True, separators=(",", ":")).encode()
    ).hexdigest()
    report["duration_seconds"] = round(time.monotonic()-started, 3)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    print(f"Saved {args.output.name}: {metadata['trainable_parameters']} parameters, {args.output.stat().st_size} bytes.", flush=True)


if __name__ == "__main__":
    main()

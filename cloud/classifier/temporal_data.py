"""Time-aligned, non-overlapping windows from the handout's Kaggle dataset."""
from pathlib import Path
import hashlib

import numpy as np
import pandas as pd

from cloud.classifier.temporal_model import SENSOR_COLUMNS, WINDOW_SIZE

CSV_SHA256 = "56ab5e861da22f7b1b4eaefa2d6fdc170f6febc2cf03d5a1883a180dab083271"


def build_windows(frame):
    required = ["date", "time", "username", "wrist", "activity", *SENSOR_COLUMNS]
    missing = sorted(set(required).difference(frame.columns))
    if missing:
        raise ValueError(f"Missing dataset columns: {missing}")
    frame = frame.copy()
    values = frame[list(SENSOR_COLUMNS)].to_numpy(dtype=np.float32)
    if not np.isfinite(values).all():
        raise ValueError("Dataset contains non-finite sensor values.")
    if not frame["activity"].isin([0, 1]).all():
        raise ValueError("Dataset activity must be 0 (Walking) or 1 (Running).")
    if not frame["wrist"].isin([0, 1]).all() or frame[required].isna().any().any():
        raise ValueError("Dataset contains invalid wrist or missing metadata.")
    clock = frame["time"].astype(str).str.extract(
        r"^(\d{1,2}):(\d{1,2}):(\d{1,2}):(\d{1,9})$"
    )
    if clock.isna().any().any():
        raise ValueError("Unexpected creator timestamp format.")
    clock = clock.astype(np.int64)
    if ((clock[0] > 23) | (clock[1] > 59) | (clock[2] > 59)).any():
        raise ValueError("Invalid dataset clock.")
    days = pd.to_datetime(frame["date"], format="%Y-%m-%d", errors="raise")
    seconds = (
        days.astype("int64").to_numpy() / 1e9
        + clock[0]*3600 + clock[1]*60 + clock[2] + clock[3]/1e9
    ).to_numpy()
    delta = np.diff(seconds, prepend=np.nan)
    boundary = (
        frame[["date", "username", "wrist", "activity"]].ne(
            frame[["date", "username", "wrist", "activity"]].shift()
        ).any(axis=1).to_numpy()
        | (delta <= 0) | (delta > 0.65)
    )
    segments = np.cumsum(boundary)
    windows, labels, groups, provenance = [], [], [], []
    rejected = 0
    # Never sort away a recording discontinuity or mix labels/wrists/users.
    for segment_id in np.unique(segments):
        indices = np.flatnonzero(segments == segment_id)
        times = seconds[indices]
        start = times[0]
        while start + (WINDOW_SIZE-1) <= times[-1] + 0.20:
            targets = start + np.arange(WINDOW_SIZE, dtype=float)
            right = np.clip(np.searchsorted(times, targets), 0, len(times)-1)
            left = np.maximum(right-1, 0)
            chosen = np.where(
                np.abs(times[left]-targets) <= np.abs(times[right]-targets),
                left, right,
            )
            if (len(np.unique(chosen)) == WINDOW_SIZE
                    and np.max(np.abs(times[chosen]-targets)) <= 0.20):
                raw_indices = indices[chosen]
                first = int(indices[0])
                windows.append(values[raw_indices])
                labels.append(int(frame["activity"].iloc[first]))
                date = days.iloc[first].strftime("%Y-%m-%d")
                user = str(frame["username"].iloc[first])
                groups.append(f"{user}/{date}")
                provenance.append({
                    "segment": int(segment_id),
                    "source_rows": raw_indices.tolist(),
                    "date_group": groups[-1],
                })
            else:
                rejected += 1
            start += float(WINDOW_SIZE)
    if not windows:
        raise ValueError("No complete time-aligned windows were available.")
    return (
        np.stack(windows), np.asarray(labels, dtype=np.int64),
        np.asarray(groups), provenance,
        {"raw_rows": len(frame), "segments": int(len(np.unique(segments))),
         "windows": len(windows), "rejected_windows": rejected,
         "nearest_sample_tolerance_seconds": 0.20,
         "recording_gap_threshold_seconds": 0.65},
    )


def day_folds(labels, groups, n_splits=5):
    unique = np.unique(groups)
    if len(unique) < n_splits:
        raise ValueError("Not enough independent recording days.")
    counts = {group: np.bincount(labels[groups == group], minlength=2) for group in unique}
    ordered = sorted(unique, key=lambda group: (-int(counts[group].sum()), str(group)))
    totals = np.bincount(labels, minlength=2).astype(float)
    fold_counts = np.zeros((n_splits, 2))
    memberships = [[] for _ in range(n_splits)]
    capacity = int(np.ceil(len(unique)/n_splits))
    for group in ordered:
        candidates = [i for i in range(n_splits) if len(memberships[i]) < capacity]
        def cost(i):
            updated = fold_counts.copy()
            updated[i] += counts[group]
            return float(np.std(updated / totals, axis=0).mean()), len(memberships[i]), i
        chosen = min(candidates, key=cost)
        memberships[chosen].append(group)
        fold_counts[chosen] += counts[group]
    result = []
    for test_days in memberships:
        test = np.flatnonzero(np.isin(groups, test_days))
        train = np.flatnonzero(~np.isin(groups, test_days))
        if not len(test) or len(np.unique(labels[train])) != 2 or len(np.unique(labels[test])) != 2:
            raise ValueError("Every training/test fold must contain both activities.")
        if set(groups[train]).intersection(groups[test]):
            raise AssertionError("Recording day leaked across a fold.")
        result.append((train, test))
    return result


def read_dataset(path):
    path = Path(path)
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    if digest != CSV_SHA256:
        raise ValueError("Dataset SHA-256 differs from the verified handout dataset version 2.")
    return pd.read_csv(path), digest

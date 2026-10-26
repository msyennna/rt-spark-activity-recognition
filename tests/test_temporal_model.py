"""Offline tests of the actual PyTorch model, checkpoint and time-window builder."""
import hashlib
import json
from pathlib import Path
import tempfile
import unittest

import numpy as np
import pandas as pd
import torch

from cloud.classifier.temporal_model import (
    ActivityGRU, SENSOR_COLUMNS, WINDOW_SIZE, load_model, rotate_windows,
)
from cloud.classifier.temporal_data import build_windows, day_folds
from cloud.classifier.predictor import predict_activity


def sample_frame(days=2):
    rows = []
    for day in range(days):
        for activity in (0, 1):
            for index in range(50):
                seconds = activity*20 + index/5
                whole = int(seconds)
                rows.append({
                    "date": f"2017-07-{day+1:02d}",
                    "time": f"00:00:{whole:02d}:{int(round((seconds-whole)*1e9))}",
                    "username": "fixture", "wrist": 1, "activity": activity,
                    **{field: (axis+1)*0.1+index*0.01 for axis, field in enumerate(SENSOR_COLUMNS)},
                })
    return pd.DataFrame(rows)


class TemporalTests(unittest.TestCase):
    def test_small_network_has_temporal_order_and_all_axes(self):
        torch.manual_seed(3)
        model = ActivityGRU()
        self.assertEqual(sum(p.numel() for p in model.parameters()), 450)
        x = torch.linspace(-0.8, 0.8, 30).reshape(1, 5, 6).requires_grad_()
        y = model(x)
        self.assertEqual(tuple(y.shape), (1, 2))
        self.assertFalse(torch.allclose(y, model(x.flip(1)), atol=1e-6))
        y.sum().backward()
        self.assertTrue(torch.all(x.grad.abs().sum(dim=(0, 1)) > 0))

    def test_rotation_preserves_sensor_magnitudes_and_time_relationship(self):
        torch.manual_seed(4)
        raw = torch.randn(7, 5, 6)
        transformed = rotate_windows(raw)
        for start in (0, 3):
            self.assertTrue(torch.allclose(
                raw[..., start:start+3].norm(dim=-1),
                transformed[..., start:start+3].norm(dim=-1), atol=1e-5))
        self.assertTrue(torch.allclose(
            (raw[:, 1, :3]*raw[:, 2, :3]).sum(-1),
            (transformed[:, 1, :3]*transformed[:, 2, :3]).sum(-1), atol=1e-5))

    def test_windows_use_timestamps_without_crossing_activity_or_day(self):
        x, y, groups, evidence, stats = build_windows(sample_frame())
        self.assertEqual(tuple(x.shape), (8, 5, 6))
        self.assertEqual(np.bincount(y).tolist(), [4, 4])
        self.assertEqual(len(set(groups)), 2)
        used = []
        for window in evidence:
            source = window["source_rows"]
            self.assertTrue(all(b-a == 5 for a, b in zip(source, source[1:])))
            used.extend(source)
        self.assertEqual(len(used), len(set(used)))
        self.assertEqual(stats["rejected_windows"], 0)

    def test_recording_gaps_are_not_silently_interpolated(self):
        frame = sample_frame(days=1)
        # Both resulting fragments are shorter than a five-reading 1 Hz window.
        fragment = frame.iloc[:15].copy()
        with self.assertRaises(ValueError):
            build_windows(fragment)
        frame.loc[0, "gyro_x"] = np.nan
        with self.assertRaisesRegex(ValueError, "non-finite"):
            build_windows(frame)

    def test_folds_hold_entire_recording_days_out(self):
        _, y, groups, _, _ = build_windows(sample_frame(days=10))
        seen = []
        for train, test in day_folds(y, groups):
            self.assertFalse(set(groups[train]).intersection(groups[test]))
            self.assertEqual(len(set(y[test])), 2)
            seen.extend(test)
        self.assertEqual(sorted(seen), list(range(len(y))))

    def test_checkpoint_roundtrip_and_hash_failure(self):
        model, metadata = load_model()
        x = torch.ones(1, WINDOW_SIZE, 6)*0.2
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/"activity_model.pt"
            torch.save(model.state_dict(), path)
            metadata["model_sha256"] = hashlib.sha256(path.read_bytes()).hexdigest()
            path.with_suffix(".json").write_text(json.dumps(metadata))
            restored, _ = load_model(path)
            self.assertTrue(torch.equal(model(x), restored(x)))
            path.write_bytes(path.read_bytes()+b"corruption")
            with self.assertRaisesRegex(ValueError, "SHA-256"):
                load_model(path)

    def test_inference_contract_and_invalid_readings(self):
        readings = [{field: 0.1 for field in SENSOR_COLUMNS} for _ in range(WINDOW_SIZE)]
        result = predict_activity(readings)
        self.assertEqual(set(result), {"activity_id", "activity", "confidence"})
        self.assertIn(result["activity_id"], (0, 1))
        self.assertTrue(0 <= result["confidence"] <= 1)
        self.assertTrue(json.dumps(result, allow_nan=False))
        with self.assertRaises(ValueError):
            predict_activity(readings[:-1])
        for bad in (float("nan"), float("inf"), True, "0.1"):
            invalid = [dict(row) for row in readings]
            invalid[0]["gyro_x"] = bad
            with self.assertRaises(ValueError):
                predict_activity(invalid)


if __name__ == "__main__":
    unittest.main()

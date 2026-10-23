"""Offline tests: temporary SQLite data and simulated clocks; no lab connection."""
from datetime import datetime, timedelta, timezone
import contextlib
import hashlib
import io
import json
from pathlib import Path
import sqlite3
import tempfile
import unittest
from unittest import mock

import timed_walk_jog as app

BASE = datetime(2026, 10, 21, 1, 0, tzinfo=timezone.utc)
PROTOCOL = {"rounds": 3, "trial_seconds": 60, "warmup_seconds": 10,
            "nominal_window_seconds": 5, "edge_margin_seconds": 1}


def record(number, seconds, activity="WALKING"):
    return {"id": number, "activity_id": app.LABELS.index(activity), "activity": activity,
            "confidence": 0.8, "created_at": (BASE + timedelta(seconds=seconds)).strftime("%Y-%m-%d %H:%M:%S")}


def trial(activity="WALKING"):
    rows = [record(index, second, activity) for index, second in enumerate(range(5, 61, 5), 1)]
    return {"trial_number": 1, "requested_activity": activity, "start_utc": app.stamp(BASE),
            "end_utc": app.stamp(BASE + timedelta(seconds=60)), "baseline_latest_record_id": 0,
            "elapsed_monotonic_seconds": 60, "completed_requested_activity": True, "records": rows}


class ScoringTests(unittest.TestCase):
    def test_time_bounds_and_wrong_predictions_are_preserved(self):
        item = trial()
        item["records"][3] = record(4, 20, "RUNNING")
        item["records"][4]["confidence"] = 0.01
        result = app.analyze_trial(item, PROTOCOL)
        self.assertTrue(result["valid_for_scoring"])
        self.assertEqual(result["eligible_record_ids"], list(range(4, 12)))
        self.assertEqual(result["eligible_count"], 8)
        self.assertEqual(result["matching_predictions"], 7)

    def test_baseline_prevents_reusing_old_ids(self):
        item = trial()
        item["baseline_latest_record_id"] = 100
        result = app.analyze_trial(item, PROTOCOL)
        self.assertEqual(result["eligible_count"], 0)
        self.assertFalse(result["valid_for_scoring"])

    def test_noncompliance_gaps_duplicates_and_clock_changes_are_flagged(self):
        with self.subTest("noncompliance"):
            item = trial()
            item["completed_requested_activity"] = False
            self.assertFalse(app.analyze_trial(item, PROTOCOL)["valid_for_scoring"])
        with self.subTest("missing stream"):
            item = trial()
            item["records"].pop(6)
            self.assertFalse(app.analyze_trial(item, PROTOCOL)["valid_for_scoring"])
        with self.subTest("duplicate"):
            item = trial()
            item["records"].append(item["records"][4])
            self.assertFalse(app.analyze_trial(item, PROTOCOL)["valid_for_scoring"])
        with self.subTest("UTC changed"):
            item = trial()
            item["elapsed_monotonic_seconds"] = 58
            self.assertFalse(app.analyze_trial(item, PROTOCOL)["valid_for_scoring"])

    def test_confusion_matrix_counts_errors_and_both_classes(self):
        walking, running = trial(), trial("RUNNING")
        walking["records"][3] = record(4, 20, "RUNNING")
        running["records"][4] = record(5, 25, "WALKING")
        for item in (walking, running):
            item["analysis"] = app.analyze_trial(item, PROTOCOL)
        result = app.aggregate({"trials": [walking, running]})
        self.assertEqual(result["confusion_matrix_rows_actual_columns_predicted"], {
            "WALKING": {"WALKING": 7, "RUNNING": 1},
            "RUNNING": {"WALKING": 1, "RUNNING": 7}})
        self.assertEqual(result["window_accuracy_percent"], 87.5)
        self.assertIsNone(app.aggregate({"trials": [walking]})["window_accuracy_percent"])
        self.assertIsNone(app.aggregate({"trials": []})["window_accuracy_percent"])

    def test_bad_database_values_are_rejected(self):
        for key, value in (("confidence", float("nan")), ("activity_id", 1), ("created_at", None)):
            with self.subTest(field=key):
                row = record(1, 0)
                row[key] = value
                with self.assertRaises(ValueError):
                    app.validate_record(row)


class DatabaseAndSessionTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.database = self.root / "cloud/database/activity.db"
        self.database.parent.mkdir(parents=True)
        with sqlite3.connect(self.database) as connection:
            connection.execute("CREATE TABLE activity_records (id INTEGER PRIMARY KEY, activity_id INTEGER, activity TEXT, confidence REAL, created_at TEXT)")
            connection.execute("INSERT INTO activity_records VALUES (?, ?, ?, ?, ?)", tuple(record(1, 0).values()))

    def test_preflight_reads_without_changing_database_or_creating_results(self):
        before = hashlib.sha256(self.database.read_bytes()).hexdigest()
        with mock.patch.object(app, "utcnow", return_value=BASE), mock.patch("sys.argv", ["timed_walk_jog.py", "--project", str(self.root), "--check-only"]), contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(app.main(), 0)
        self.assertEqual(hashlib.sha256(self.database.read_bytes()).hexdigest(), before)
        self.assertFalse((self.root / "docs").exists())
        with self.assertRaises(ValueError):
            app.preflight(self.database, BASE + timedelta(seconds=16))
        with self.assertRaises(ValueError):
            app.preflight(self.database, BASE - timedelta(seconds=3))

    def simulate(self, interrupt=False):
        clock = {"seconds": 0.0, "timers": 0, "next_id": 2}
        def now():
            return BASE + timedelta(seconds=clock["seconds"])
        def sleep(seconds):
            clock["seconds"] += seconds
        def timer(seconds):
            clock["timers"] += 1
            if interrupt:
                sleep(20)
                raise KeyboardInterrupt()
            actual = app.LABELS[(clock["timers"] - 1) % 2]
            start = clock["seconds"]
            with sqlite3.connect(self.database) as connection:
                for second in range(5, 61, 5):
                    predicted = app.LABELS[1 - app.LABELS.index(actual)] if second == 20 else actual
                    row = record(clock["next_id"], start + second, predicted)
                    connection.execute("INSERT INTO activity_records VALUES (?, ?, ?, ?, ?)", tuple(row.values()))
                    clock["next_id"] += 1
            sleep(seconds)
        replies = iter(["same hand and orientation"] + [reply for _ in range(6) for reply in ("", "y")])
        output = io.StringIO()
        with mock.patch.object(app, "utcnow", side_effect=now), mock.patch.object(app.time, "monotonic", side_effect=lambda: clock["seconds"]), mock.patch.object(app.time, "sleep", side_effect=sleep), mock.patch.object(app, "run_timer", side_effect=timer), mock.patch("builtins.input", side_effect=lambda prompt: next(replies)), contextlib.redirect_stdout(output):
            status = app.run_session(self.root, 3, 60, 10)
        folders = list((self.root / "docs/testing").iterdir())
        self.assertEqual(len(folders), 1)
        data = json.loads((folders[0] / "session.json").read_text())
        self.assertTrue((folders[0] / "summary.md").is_file())
        return status, data, output.getvalue()

    def test_full_six_trial_flow_saves_real_errors_without_timing_wait(self):
        status, data, output = self.simulate()
        self.assertEqual(status, 0)
        self.assertEqual(data["status"], "COMPLETE")
        self.assertEqual(data["metrics"]["scored_windows"], 48)
        self.assertEqual(data["metrics"]["correct_windows"], 42)
        self.assertEqual(data["metrics"]["window_accuracy_percent"], 87.5)
        self.assertEqual(len(data["trials"]), 6)
        self.assertIn("RESULTS FOLDER:", output)

    def test_interruption_preserves_partial_evidence_without_claiming_accuracy(self):
        status, data, _ = self.simulate(interrupt=True)
        self.assertEqual(status, 2)
        self.assertEqual(data["status"], "INTERRUPTED")
        self.assertEqual(len(data["trials"]), 1)
        self.assertFalse(data["trials"][0]["analysis"]["valid_for_scoring"])
        self.assertIsNone(data["metrics"]["window_accuracy_percent"])


if __name__ == "__main__":
    unittest.main(verbosity=2)

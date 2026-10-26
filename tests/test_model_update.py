"""Verify deployment scope and restoring existing/absent files after failure."""
import contextlib
import importlib.util
from io import StringIO
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

path = Path(__file__).resolve().parents[1]/"tools/provisioning/apply_pytorch_update.py"
spec = importlib.util.spec_from_file_location("model_update", path)
update = importlib.util.module_from_spec(spec)
spec.loader.exec_module(update)


class ModelUpdateTests(unittest.TestCase):
    def test_service_from_another_project_is_rejected(self):
        project = Path("/home/ubuntu/rt-spark-activity-recognition")
        wrong = subprocess.CompletedProcess([], 0,
            "ExecStart={ path=/home/other/cloud/mqtt/mqtt_classifier.py; }\n"
            "WorkingDirectory=/home/other\n", "")
        correct = subprocess.CompletedProcess([], 0,
            "ExecStart={ path=/home/ubuntu/rt-spark-activity-recognition/.venv/bin/python; "
            "argv[]=python cloud/mqtt/mqtt_classifier.py; }\n"
            "WorkingDirectory=/home/ubuntu/rt-spark-activity-recognition\n", "")
        with patch.object(update.subprocess, "run", return_value=wrong):
            self.assertFalse(update.service_matches("activity-classifier.service", project))
        with patch.object(update.subprocess, "run", return_value=correct):
            self.assertTrue(update.service_matches("activity-classifier.service", project))

    def check_restore(self, failure):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            project, stage = root/"project", root/"stage"
            old = set(update.TARGETS[:3])
            for name in update.TARGETS:
                (stage/name).parent.mkdir(parents=True, exist_ok=True)
                (stage/name).write_text("replacement")
                if name in old:
                    (project/name).parent.mkdir(parents=True, exist_ok=True)
                    (project/name).write_text("original")
            def control(action, service):
                if action == "start" and control.starts == 0:
                    control.starts += 1
                    raise failure
            control.starts = 0
            with patch.object(update, "STAGE", stage), \
                 patch.object(update, "run"), \
                 patch.object(update, "validate_model"), \
                 patch.object(update, "latest_id", return_value=10), \
                 patch.object(update, "control", side_effect=control), \
                 contextlib.redirect_stdout(StringIO()):
                with self.assertRaises(type(failure)):
                    update.apply(project, "activity-classifier.service",
                                 project/".venv/bin/python", {"model_sha256": "test"})
            for name in update.TARGETS:
                if name in old:
                    self.assertEqual((project/name).read_text(), "original")
                else:
                    self.assertFalse((project/name).exists())

    def test_start_failure_restores_old_files_and_removes_new_files(self):
        self.check_restore(RuntimeError("simulated failed restart"))

    def test_interrupt_restores_old_files_and_removes_new_files(self):
        self.check_restore(KeyboardInterrupt())

    def test_success_requires_fresh_record_and_matching_model_startup_hash(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            project, stage = root/"project", root/"stage"
            for name in update.TARGETS:
                (stage/name).parent.mkdir(parents=True, exist_ok=True)
                (stage/name).write_text("replacement")
            calls = []
            def command(args, **kwargs):
                calls.append(args)
                # Both attempts see a fresh DB ID, but only the second journal
                # contains the expected loaded model's startup hash.
                journal_count = sum("journalctl" in call for call in calls)
                stdout = "expected-model-hash" if journal_count >= 2 else "old-model"
                return subprocess.CompletedProcess(args, 0, stdout, "")
            with patch.object(update, "STAGE", stage), \
                 patch.object(update, "run"), \
                 patch.object(update, "validate_model"), \
                 patch.object(update, "latest_id", side_effect=[10, 11, 12]), \
                 patch.object(update, "control"), \
                 patch.object(update.subprocess, "run", side_effect=command), \
                 patch.object(update.time, "sleep"), \
                 contextlib.redirect_stdout(StringIO()):
                update.apply(project, "activity-classifier.service",
                             project/".venv/bin/python", {"model_sha256": "expected-model-hash"})
            audits = list((root/"rtspark-model-backups").glob("*/deployment.json"))
            self.assertEqual(len(audits), 1)
            self.assertEqual(json.loads(audits[0].read_text())["observed_new_database_id"], 12)
            self.assertEqual(sum("journalctl" in call for call in calls), 2)


if __name__ == "__main__":
    unittest.main()

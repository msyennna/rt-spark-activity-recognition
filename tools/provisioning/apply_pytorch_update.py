"""Inspect or apply the PyTorch update on Ubuntu, with source-file rollback."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import shutil
import sqlite3
import subprocess
import sys
import time
import uuid

STAGE = Path(__file__).resolve().parents[2]
TARGETS = [
    "cloud/classifier/temporal_model.py", "cloud/classifier/temporal_data.py",
    "cloud/classifier/train_model.py", "cloud/classifier/predictor.py",
    "cloud/classifier/activity_model.pt", "cloud/classifier/activity_model.json",
    "cloud/mqtt/mqtt_classifier.py",
]


def run(args, **kwargs):
    return subprocess.run(list(map(str, args)), check=True, **kwargs)


def verify_stage():
    manifest = json.loads((STAGE/"pytorch-update-manifest.json").read_text())
    if manifest.get("format") != "rtspark-pytorch-update-v1" or manifest.get("targets") != TARGETS:
        raise ValueError("Unexpected update manifest or target list.")
    allowed = set(TARGETS + [
        "cloud/api/app.py", "cloud/database/database.py",
        "tests/test_temporal_model.py", "tests/test_cloud_temporal_contract.py",
        "tools/provisioning/apply_pytorch_update.py",
    ])
    if set(manifest["sha256"]) != allowed:
        raise ValueError("Unexpected staged file list.")
    for name, expected in manifest["sha256"].items():
        if hashlib.sha256((STAGE/name).read_bytes()).hexdigest() != expected:
            raise ValueError(f"Staged SHA-256 mismatch: {name}")
    model = json.loads((STAGE/"cloud/classifier/activity_model.json").read_text())
    if model.get("framework") != "PyTorch" or model.get("model_sha256") != manifest["sha256"]["cloud/classifier/activity_model.pt"]:
        raise ValueError("PyTorch model metadata mismatch.")
    return model


def validate_model(python):
    run([python, "-m", "unittest", "discover", "-s", "tests",
         "-p", "test_*temporal*.py", "-v"], cwd=STAGE)


def service_matches(name, project):
    if not re.fullmatch(r"[A-Za-z0-9_.@:-]+\.service", name):
        return False
    result = subprocess.run(["systemctl", "show", name, "--property=ExecStart",
                             "--property=WorkingDirectory"],
                            text=True, capture_output=True, check=False)
    properties = dict(line.split("=", 1) for line in result.stdout.splitlines() if "=" in line)
    command = properties.get("ExecStart", "")
    working = properties.get("WorkingDirectory", "").removeprefix("-")
    return result.returncode == 0 and (
        str(project/"cloud/mqtt/mqtt_classifier.py") in command
        or (working and Path(working).resolve() == project
            and "cloud/mqtt/mqtt_classifier.py" in command)
    )


def find_services(project):
    result = run(["systemctl", "list-unit-files", "--type=service", "--no-legend", "--no-pager"],
                 text=True, capture_output=True)
    names = [line.split()[0] for line in result.stdout.splitlines() if line.strip()]
    return [name for name in names
            if re.search(r"rtspark|activity|classifier", name, re.I)
            and service_matches(name, project)]


def latest_id(project):
    database = project/"cloud/database/activity.db"
    with sqlite3.connect(f"file:{database}?mode=ro", uri=True) as connection:
        row = connection.execute("SELECT MAX(id) FROM activity_records").fetchone()
    return int(row[0] or 0)


def control(action, service):
    prefix = [] if __import__("os").geteuid() == 0 else ["sudo", "-n"]
    run(prefix + ["systemctl", action, service])


def apply(project, service, python, metadata):
    # Only CPU PyTorch is added. Existing Flask/MQTT/DB packages are not replaced.
    run([python, "-m", "pip", "install", "--index-url",
         "https://download.pytorch.org/whl/cpu", "torch==2.14.1+cpu"])
    validate_model(python)
    baseline = latest_id(project)
    suffix = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")+"_"+uuid.uuid4().hex[:6]
    backup = project.parent/"rtspark-model-backups"/suffix
    backup.mkdir(parents=True, mode=0o700)
    existed = {}
    for name in TARGETS:
        destination = project/name
        existed[name] = destination.exists()
        if existed[name]:
            saved = backup/name
            saved.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(destination, saved)
    journal_since = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")
    try:
        control("stop", service)
        for name in TARGETS:
            destination = project/name
            destination.parent.mkdir(parents=True, exist_ok=True)
            pending = destination.with_name(destination.name+".pytorch-pending")
            shutil.copy2(STAGE/name, pending)
            pending.replace(destination)
        control("start", service)
        first_new = None
        for attempt in range(45):
            active = subprocess.run(["systemctl", "is-active", "--quiet", service], check=False)
            if active.returncode != 0:
                raise RuntimeError("Classifier service is not active.")
            current = latest_id(project)
            prefix = [] if __import__("os").geteuid() == 0 else ["sudo", "-n"]
            journal = subprocess.run(prefix + ["journalctl", "-u", service, "--since",
                                     journal_since, "--no-pager", "-o", "cat"],
                                     text=True, capture_output=True, check=False)
            if (current > baseline and journal.returncode == 0
                    and metadata["model_sha256"] in journal.stdout):
                first_new = current
                break
            if attempt % 5 == 0:
                print("Waiting for a new prediction from the powered board...", flush=True)
            time.sleep(1)
        if first_new is None:
            raise RuntimeError("No fresh prediction with a matching model startup hash within 45 seconds.")
        audit = {
            "service": service, "model_sha256": metadata["model_sha256"],
            "baseline_database_id": baseline, "observed_new_database_id": first_new,
            "completed_at_utc": datetime.now(timezone.utc).isoformat(),
            "backup": str(backup), "targets": TARGETS,
        }
        (backup/"deployment.json").write_text(json.dumps(audit, indent=2)+"\n")
        print(json.dumps({"status": "PASS", **audit}, indent=2))
    except BaseException:
        control("stop", service)
        for name in TARGETS:
            destination = project/name
            if existed[name]:
                shutil.copy2(backup/name, destination)
            elif destination.exists():
                destination.unlink()
        control("start", service)
        print(f"Previous source/model files restored from {backup}.")
        print("The added CPU PyTorch package remains installed; the live database was not replaced.")
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path, default=Path.home()/"rt-spark-activity-recognition")
    parser.add_argument("--service")
    parser.add_argument("--apply", action="store_true")
    parser.add_argument("--stage-only", action="store_true")
    args = parser.parse_args()
    metadata = verify_stage()
    if args.stage_only:
        validate_model(sys.executable)
        print("PASS: staged hashes, actual PyTorch model and temporary database/API tests.")
        return
    project = args.project.resolve()
    python = project/".venv/bin/python"
    if not python.is_file() or not (project/"cloud/mqtt/mqtt_classifier.py").is_file():
        raise SystemExit("Expected existing Ubuntu project and .venv/bin/python were not found.")
    services = [args.service] if args.service else find_services(project)
    if len(services) != 1 or not service_matches(services[0], project):
        raise SystemExit(f"Specify the actual classifier --service; matching candidates: {services}")
    if not args.apply:
        print(json.dumps({"mode": "read-only inspection", "project": str(project),
                          "classifier_service": services[0],
                          "staged_model_sha256": metadata["model_sha256"],
                          "baseline_database_id": latest_id(project)}, indent=2))
        return
    apply(project, services[0], python, metadata)


if __name__ == "__main__":
    main()

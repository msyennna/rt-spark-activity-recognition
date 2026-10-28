"""Run independently timed walking/jogging trials on the Ubuntu database host.

Standard library only. Opens activity.db in read-only mode. Writes a new JSON
session and Markdown summary; never changes the model, services, or database.
"""
from __future__ import annotations

import argparse
from contextlib import closing
from datetime import datetime, timedelta, timezone
import json
import math
from pathlib import Path
import sqlite3
import time
import uuid

UTC = timezone.utc
LABELS = ("WALKING", "RUNNING")
VERSION = "1.0"


def utcnow():
    return datetime.now(UTC)


def stamp(value):
    return value.astimezone(UTC).isoformat(timespec="microseconds").replace("+00:00", "Z")


def unstamp(value):
    parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
    if parsed.tzinfo is None:
        raise ValueError("A trial timestamp has no UTC offset.")
    return parsed.astimezone(UTC)


def record_time(row):
    value = row["created_at"]
    if not isinstance(value, str):
        raise ValueError("Unexpected database timestamp type.")
    parsed = datetime.strptime(value, "%Y-%m-%d %H:%M:%S")
    if parsed.strftime("%Y-%m-%d %H:%M:%S") != value:
        raise ValueError("Unexpected database timestamp format.")
    return parsed.replace(tzinfo=UTC)


def validate_record(row):
    value = dict(row)
    if type(value["id"]) is not int or value["id"] <= 0:
        raise ValueError("Invalid prediction ID.")
    if value["activity"] not in LABELS or value["activity_id"] != LABELS.index(value["activity"]):
        raise ValueError("A database activity label does not match its class ID.")
    confidence = value["confidence"]
    if type(confidence) not in (int, float) or not math.isfinite(confidence) or not 0 <= confidence <= 1:
        raise ValueError("Invalid database confidence.")
    record_time(value)
    return value


def read_database(database, start=None, end=None):
    database = Path(database).resolve()
    if not database.is_file():
        raise ValueError("Database not found. Run this on Ubuntu, where the classifier writes activity.db.")
    with closing(sqlite3.connect(database.as_uri() + "?mode=ro", uri=True, timeout=3)) as connection:
        connection.row_factory = sqlite3.Row
        query = "SELECT id, activity_id, activity, confidence, created_at FROM activity_records"
        if start is None:
            rows = connection.execute(query + " ORDER BY id DESC LIMIT 1").fetchall()
        else:
            rows = connection.execute(
                query + " WHERE created_at >= ? AND created_at <= ? ORDER BY id",
                (start.strftime("%Y-%m-%d %H:%M:%S"), end.strftime("%Y-%m-%d %H:%M:%S")),
            ).fetchall()
    return [validate_record(row) for row in rows]


def preflight(database, now=None):
    rows = read_database(database)
    if not rows:
        raise ValueError("No predictions yet. Power the board and wait for LIVE before starting.")
    latest = rows[0]
    age = ((now or utcnow()) - record_time(latest)).total_seconds()
    if age < -2:
        raise ValueError("A prediction timestamp is ahead of the server clock. Check UTC before testing.")
    if age > 15:
        raise ValueError("Latest prediction is older than 15 seconds. Power the board and wait for fresh predictions.")
    return latest, age


def analyze_trial(trial, protocol):
    start, end = unstamp(trial["start_utc"]), unstamp(trial["end_utc"])
    nominal_window = protocol["nominal_window_seconds"]
    margin = protocol["edge_margin_seconds"]
    earliest_end = start + timedelta(seconds=protocol["warmup_seconds"] + nominal_window + margin)
    latest_end = end - timedelta(seconds=margin)
    baseline_id = trial.get("baseline_latest_record_id", 0)
    eligible = [row for row in trial["records"] if row["id"] > baseline_id and earliest_end <= record_time(row) <= latest_end]
    eligible.sort(key=lambda row: (row["created_at"], row["id"]))
    issues = []
    if trial.get("completed_requested_activity") is not True:
        issues.append("Participant did not confirm completing the requested activity throughout the interval.")
    elapsed = trial["elapsed_monotonic_seconds"]
    if elapsed < protocol["trial_seconds"] - 0.25 or elapsed > protocol["trial_seconds"] + 1:
        issues.append("Timer did not run for the planned duration.")
    if abs((end - start).total_seconds() - elapsed) > 1:
        issues.append("Server UTC changed relative to the monotonic timer.")
    interval = max(0, protocol["trial_seconds"] - protocol["warmup_seconds"] - nominal_window - 2 * margin)
    minimum = math.floor(interval / nominal_window)
    maximum = math.ceil(interval / nominal_window) + 1
    if len(eligible) < minimum or len(eligible) > maximum:
        issues.append("Prediction count does not cover the expected interval at the nominal five-second rate.")
    if len({row["id"] for row in trial["records"]}) != len(trial["records"]):
        issues.append("Duplicate prediction IDs were supplied.")
    intervals = [(record_time(right) - record_time(left)).total_seconds() for left, right in zip(eligible, eligible[1:])]
    if any(value < 3 or value > 8 for value in intervals):
        issues.append("Prediction cadence contains a gap or unexpectedly rapid results.")
    correct = sum(row["activity"] == trial["requested_activity"] for row in eligible)
    return {
        "selection_method": "Independent timed interval; selection never uses predicted class or confidence.",
        "earliest_eligible_prediction_end_utc": stamp(earliest_end),
        "latest_eligible_prediction_end_utc": stamp(latest_end),
        "eligible_record_ids": [row["id"] for row in eligible],
        "eligible_count": len(eligible),
        "matching_predictions": correct,
        "quality_issues": issues,
        "valid_for_scoring": not issues,
    }


def aggregate(session):
    matrix = {actual: {predicted: 0 for predicted in LABELS} for actual in LABELS}
    valid_trials = 0
    for trial in session["trials"]:
        analysis = trial.get("analysis", {})
        if not analysis.get("valid_for_scoring"):
            continue
        valid_trials += 1
        selected = set(analysis["eligible_record_ids"])
        for row in trial["records"]:
            if row["id"] in selected:
                matrix[trial["requested_activity"]][row["activity"]] += 1
    support = {label: sum(matrix[label].values()) for label in LABELS}
    total = sum(support.values())
    correct = sum(matrix[label][label] for label in LABELS)
    return {
        "confusion_matrix_rows_actual_columns_predicted": matrix,
        "valid_trial_count": valid_trials,
        "scored_windows": total,
        "correct_windows": correct,
        "support_by_actual_activity": support,
        "window_accuracy_percent": 100 * correct / total if all(support.values()) else None,
        "correct_identification_percent_by_activity": {
            label: 100 * matrix[label][label] / support[label] if support[label] else None for label in LABELS
        },
    }


def atomic_text(path, content):
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(content, encoding="utf-8")
    temporary.replace(path)


def save_session(folder, session):
    session["metrics"] = aggregate(session)
    atomic_text(folder / "session.json", json.dumps(session, indent=2, allow_nan=False) + "\n")
    metrics = session["metrics"]
    matrix = metrics["confusion_matrix_rows_actual_columns_predicted"]
    score = metrics["window_accuracy_percent"]
    score_text = "Not available: both activities need valid windows." if score is None else f"{score:.1f}% ({metrics['correct_windows']}/{metrics['scored_windows']} scored windows)."
    lines = [
        "# Independently timed walking and jogging test", "",
        "Session: `" + session["session_id"] + "`", "",
        "Status: **" + session["status"] + "**", "",
        "Window accuracy on valid recorded intervals: " + score_text, "",
        "A partial or incomplete session does not fulfill the planned test protocol.", "",
        "| Actual activity | Predicted WALKING | Predicted RUNNING |",
        "| --- | ---: | ---: |",
        f"| WALKING | {matrix['WALKING']['WALKING']} | {matrix['WALKING']['RUNNING']} |",
        f"| RUNNING | {matrix['RUNNING']['WALKING']} | {matrix['RUNNING']['RUNNING']} |", "",
        "## Trials", "",
        "| Trial | Requested activity | Confirmed by participant | Eligible windows | Matching predictions | Data quality |",
        "| --- | --- | --- | ---: | ---: | --- |",
    ]
    for trial in session["trials"]:
        analysis = trial.get("analysis", {})
        issues = analysis.get("quality_issues", [])
        quality = "; ".join(issues) if issues else "Valid" if analysis.get("valid_for_scoring") else "Not finished"
        lines.append(f"| {trial['trial_number']} | {trial['requested_activity']} | {trial.get('completed_requested_activity')} | {analysis.get('eligible_count', 0)} | {analysis.get('matching_predictions', 0)} | {quality} |")
    lines.extend([
        "", "## Conditions and method", "",
        "- Board placement: " + session["board_placement"].replace("\n", " "),
        "- Protocol and requested activity order were fixed before predictions were inspected.",
        "- Start/end UTC times were recorded on the same Ubuntu host that timestamps predictions.",
        "- A monotonic clock controlled duration; UTC/monotonic disagreements flag the interval.",
        "- Predictions were not displayed until all requested trials finished or the session stopped.",
        "- Selection uses time only. Incorrect predictions and low confidence do not cause exclusion.",
        "- Participant-confirmation and data-quality exclusions retain raw rows and reasons in session.json.",
        "", "## Limits", "",
        "This is a short, non-randomized, single-participant test. It does not establish accuracy across people or environments.",
        "Database timestamps mark prediction completion, not individual sensor acquisition. Window starts are estimated using the nominal five-second window. Warm-up and edge exclusions reduce transition ambiguity but do not measure sensor/network delay.",
        "Confidence is not accuracy. The model has WALKING/RUNNING labels only. No resting class or training cross-validation is evaluated here.",
        "", "Machine-readable evidence: [session.json](session.json).", "",
    ])
    if session.get("stop_reason"):
        lines.extend(["Stop reason: " + session["stop_reason"], ""])
    atomic_text(folder / "summary.md", "\n".join(lines))


def cue(message):
    print("\a" + message, flush=True)


def run_timer(seconds):
    deadline = time.monotonic() + seconds
    last_remaining = None
    while True:
        left = max(0, math.ceil(deadline - time.monotonic()))
        if left == 0:
            return
        if left != last_remaining and (left % 5 == 0 or left <= 3):
            print(f"{left} seconds remaining", flush=True)
            last_remaining = left
        time.sleep(min(0.1, max(0, deadline - time.monotonic())))


def run_session(project, rounds, seconds, warmup):
    database = project / "cloud/database/activity.db"
    preflight(database)
    print("Perform the requested activity throughout each interval. Keep the same board placement.")
    print("Minimize the app's prediction view until the session finishes; predictions here remain hidden.")
    print("Follow START/STOP cues. Terminal bells may be muted, so keep the timer visible or have a helper read it.")
    placement = input("Describe board placement (for example, held in right hand): ").strip() or "Not recorded"
    session_id = "timed_walk_jog_" + utcnow().strftime("%Y%m%dT%H%M%SZ") + "_" + uuid.uuid4().hex[:6]
    folder = project / "docs/testing" / session_id
    folder.mkdir(parents=True, exist_ok=False)
    protocol = {"rounds": rounds, "trial_seconds": seconds, "warmup_seconds": warmup,
                "nominal_window_seconds": 5, "edge_margin_seconds": 1,
                "planned_activities": list(LABELS) * rounds}
    session = {"schema_version": 1, "tool_version": VERSION, "session_id": session_id,
               "status": "IN_PROGRESS", "board_placement": placement, "protocol": protocol,
               "created_utc": stamp(utcnow()), "trials": []}
    save_session(folder, session)
    active = None
    started_monotonic = None
    try:
        for number, activity in enumerate(protocol["planned_activities"], 1):
            print(f"\nTrial {number}/{rounds * 2}: {'walk normally' if activity == 'WALKING' else 'jog'} for {seconds} seconds.")
            input("Press Enter when ready for the 3-second countdown. Rest between trials if needed. ")
            latest_before_trial, _ = preflight(database)
            for value in (3, 2, 1):
                print(value, flush=True)
                time.sleep(1)
            started_monotonic = time.monotonic()
            active = {"trial_number": number, "requested_activity": activity,
                      "start_utc": stamp(utcnow()), "planned_end_utc": None,
                      "baseline_latest_record_id": latest_before_trial["id"],
                      "completed_requested_activity": None, "records": []}
            active["planned_end_utc"] = stamp(unstamp(active["start_utc"]) + timedelta(seconds=seconds))
            session["trials"].append(active)
            save_session(folder, session)
            cue("START " + ("WALKING" if activity == "WALKING" else "JOGGING"))
            remaining = max(0, seconds - (time.monotonic() - started_monotonic))
            run_timer(remaining)
            active["end_utc"] = stamp(utcnow())
            active["elapsed_monotonic_seconds"] = time.monotonic() - started_monotonic
            cue("STOP. You may rest now.")
            while True:
                answer = input("Did you perform the requested activity throughout the interval? Type y or n: ").strip().lower()
                if answer in ("y", "n"):
                    active["completed_requested_activity"] = answer == "y"
                    break
            if answer == "n":
                active["participant_note"] = input("Brief reason (predictions are still hidden): ").strip()
            active["records"] = read_database(database, unstamp(active["start_utc"]) - timedelta(seconds=5), unstamp(active["end_utc"]) + timedelta(seconds=5))
            active["analysis"] = analyze_trial(active, protocol)
            save_session(folder, session)
            print("Trial saved. Prediction comparison remains hidden until the session finishes.")
            active = None
        valid = aggregate(session)["valid_trial_count"]
        session["status"] = "COMPLETE" if valid == rounds * 2 else "INCOMPLETE"
    except (KeyboardInterrupt, EOFError, OSError, ValueError, sqlite3.Error) as error:
        session["status"] = "INTERRUPTED" if isinstance(error, (KeyboardInterrupt, EOFError)) else "STOPPED"
        session["stop_reason"] = type(error).__name__ + (": " + str(error) if str(error) else "")
        if active is not None:
            active.setdefault("end_utc", stamp(utcnow()))
            active.setdefault("elapsed_monotonic_seconds", time.monotonic() - started_monotonic)
            try:
                active["records"] = read_database(database, unstamp(active["start_utc"]) - timedelta(seconds=5), unstamp(active["end_utc"]) + timedelta(seconds=5))
                active["analysis"] = analyze_trial(active, protocol)
            except (OSError, ValueError, sqlite3.Error) as read_error:
                active["read_error"] = type(read_error).__name__ + ": " + str(read_error)
    session["finished_utc"] = stamp(utcnow())
    save_session(folder, session)
    print("\n" + (folder / "summary.md").read_text(encoding="utf-8"))
    print("RESULTS FOLDER: " + str(folder))
    return 0 if session["status"] == "COMPLETE" else 2


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path, default=Path.home() / "rt-spark-activity-recognition")
    parser.add_argument("--check-only", action="store_true")
    parser.add_argument("--rounds", type=int, choices=range(1, 6), default=3)
    parser.add_argument("--seconds", type=int, default=60)
    parser.add_argument("--warmup", type=int, default=10)
    args = parser.parse_args()
    if args.warmup < 5 or args.seconds - args.warmup - 7 < 20:
        parser.error("Allow at least five warm-up seconds and twenty eligible seconds after window/edge exclusions.")
    try:
        print("RTSpark timed validation v" + VERSION)
        project = args.project.resolve()
        if args.check_only:
            latest, age = preflight(project / "cloud/database/activity.db")
            print(f"PASS: database opened read-only; latest prediction #{latest['id']} is {age:.1f} seconds old.")
            print("READY: live sensor predictions are available. No trial or result files were created.")
            return 0
        return run_session(project, args.rounds, args.seconds, args.warmup)
    except (OSError, ValueError, sqlite3.Error) as error:
        print("STOPPED: " + str(error))
        return 1
    except (KeyboardInterrupt, EOFError):
        print("Stopped before a timed session began.")
        return 130


if __name__ == "__main__":
    raise SystemExit(main())

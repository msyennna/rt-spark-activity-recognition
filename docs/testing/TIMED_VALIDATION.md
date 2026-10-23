# Independently timed walking and jogging validation

The earlier 40-second walking/40-second jogging record remains an exploratory
result: exact movement boundaries were reconstructed afterward. This tool fixes
the requested labels and records the start/end times before selecting predictions.

## Protocol

- Run the timer on the Ubuntu VM that writes `activity_records`.
- Use the directly connected board with portable power and consistent placement.
  Keep the legacy Windows publisher stopped so there is one sensor source.
- Three rounds alternate WALKING and RUNNING: six 60-second trials in total.
- Jog during RUNNING trials. Perform the requested movement throughout each
  START-to-STOP interval. Rest between trials as needed; the next one waits for Enter.
- Keep the terminal timer visible or have a helper read its cues. Terminal bells
  may be muted. Minimize the app's predicted-activity display during the session.
- The first ten seconds are settling time. To allow a full nominal five-second
  window within the remaining interval, prediction completion timestamps must
  also fit one-second edge margins. This normally retains eight or nine predictions
  per trial, depending on the classifier's phase.
- Confirm whether you actually performed the requested activity before seeing
  the prediction comparison. All requested trials and exclusions remain recorded.

This takes six minutes of movement plus countdowns and the breaks you choose.
Changing duration or rounds creates a different protocol, which is stored in
the session evidence. Do not choose durations after looking at the predicted labels.

## Windows integration worktree

These files are included in `integration/stm32cubemx`:

- `tools/evaluation/timed_walk_jog.py`
- `tools/evaluation/test_timed_walk_jog.py`
- `docs/testing/TIMED_VALIDATION.md`

From Windows PowerShell, update the integration worktree and run the offline tests:

```powershell
Set-Location "$env:USERPROFILE\rt-spark-activity-recognition\.worktrees\stm32cubemx"
git pull --ff-only origin integration/stm32cubemx
python .\tools\evaluation\test_timed_walk_jog.py
```

The helper and instructions are already committed as a preparation milestone.
Eight offline tests passed during the October 23 integration update. The tests
use temporary databases and dummy data, including deliberately incorrect
predictions. Their synthetic scores are not measurements of the actual board.

## Transfer and preflight

From Windows PowerShell:

```powershell
scp -i "$env:USERPROFILE\Downloads\rtspark-yena.pem" .\tools\evaluation\timed_walk_jog.py ubuntu@15.134.214.242:/home/ubuntu/timed_walk_jog.py
ssh -i "$env:USERPROFILE\Downloads\rtspark-yena.pem" ubuntu@15.134.214.242
```

Then, in Ubuntu:

```bash
python3 /home/ubuntu/timed_walk_jog.py --project /home/ubuntu/rt-spark-activity-recognition --check-only
```

The default project is `$HOME/rt-spark-activity-recognition`. The preflight opens
`cloud/database/activity.db` with SQLite `mode=ro`, requires a recent prediction,
and creates no result files. It does not change services, dependencies, firmware,
the model, or the database. Use `--project /path/to/repo` for a different location.

If predictions are stale, power the board, allow Wi-Fi/MQTT to reconnect, and
retry the preflight. Do not start a duplicate classifier to make the test run.

## Run and save the result milestone

In Ubuntu, when the preflight passes and you can follow the movement cues:

```bash
python3 /home/ubuntu/timed_walk_jog.py --project /home/ubuntu/rt-spark-activity-recognition
```

Describe board placement, follow the countdown and START/STOP cues, and answer
the participation question after each trial. Requested activities and times are
saved as the session proceeds. The summary appears at the end, or when stopped.
An interrupted session retains its partial evidence.

The tool prints `RESULTS FOLDER:` followed by a new directory inside
`docs/testing/timed_walk_jog_<UTC timestamp>_<suffix>`. Each directory contains
`session.json` with the protocol, timestamps, raw rows, selection rule and quality
checks, plus `summary.md` with per-trial counts and a confusion matrix. Rows are
actual activities; columns are predicted activities. Accuracy divides correct
windows by scored windows, and is unavailable until both actual classes have
valid windows. An INCOMPLETE session does not fulfill the planned six-trial protocol.

In a separate Windows PowerShell window, copy the exact result directory printed
by the helper. Replace the example folder name below with that printed name:

```powershell
Set-Location "$env:USERPROFILE\rt-spark-activity-recognition\.worktrees\stm32cubemx"
$rtsparkTrialFolder = "timed_walk_jog_REPLACE_WITH_PRINTED_NAME"
scp -r -i "$env:USERPROFILE\Downloads\rtspark-yena.pem" "ubuntu@15.134.214.242:/home/ubuntu/rt-spark-activity-recognition/docs/testing/$rtsparkTrialFolder" .\docs\testing\
```

Review `summary.md` and `session.json`, then commit and push that result directory
as a separate milestone on `integration/stm32cubemx`. Retain previous and
incomplete sessions; do not overwrite them to hide poor predictions. Only the
helper is transferred to Ubuntu; this procedure does not update server source
or restart the working services.

## Eligibility and limitations

Selection uses the independent timer interval and a pre-trial latest-record ID.
It does not use predicted class or confidence. Incorrect and low-confidence
predictions stay in the score. Missing participation confirmation, incomplete
timing, UTC jumps, duplicate IDs, unexpected counts, and irregular cadence flag
an interval. Raw evidence and the reasons are retained. Partial metrics cover
only valid intervals and do not imply that the entire protocol passed.

The server timer uses a monotonic clock for elapsed time. Database access uses
read-only URI mode. Reference: Python's official [time documentation](https://docs.python.org/3/library/time.html#time.monotonic)
and [SQLite URI documentation](https://docs.python.org/3/library/sqlite3.html#how-to-work-with-sqlite-uris).

Database timestamps are prediction completion times, not per-reading acquisition
times. Estimated windows assume the configured five readings at approximately
one hertz. Settling time and margins reduce boundary ambiguity but do not measure
transport delay. This is a short, fixed-order, single-participant evaluation; it
does not replace subject-independent model validation or establish performance
across people, placements, or environments. The model has no resting class.

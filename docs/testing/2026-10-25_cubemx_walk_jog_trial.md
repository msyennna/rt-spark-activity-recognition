# STM32CubeMX walking and jogging trial: October 25, 2026

## Result and provenance

The user supplied the Ubuntu terminal transcript of timed validation v1.0 for
session `timed_walk_jog_20261025T153506Z_0abffb`. The helper reported
**COMPLETE**, with six Valid trials and **51/52 correct eligible windows
(98.1%)**. The per-trial totals and reported confusion matrix agree.

This is a record of the supplied terminal summary. The reviewer did not observe
the movements or operate the board. The original `session.json` and
`summary.md` have not yet been supplied, so raw timestamps, prediction IDs,
window selection and quality flags have not been independently recomputed.
No raw records or missing times are reconstructed in this report.

The repository head inspected before this result entry was
`423a0b41cef9bb0470c06ba971214e4c50fb0321`. The reviewed evaluator source reports
version 1.0, matching the transcript. The exact VM script, flashed firmware and
model hashes were not supplied.

## Reported protocol

- One participant; board placement: **held in right hand**.
- Three rounds of WALKING followed by RUNNING, six trials total.
- WALKING meant normal walking; RUNNING trials requested jogging.
- Each requested interval lasted 60 seconds, with a three-second countdown.
- The participant answered `y` after every interval.
- Breaks were allowed before each subsequent countdown.
- Requested activity order and server-side time intervals were fixed before
  comparison with predictions.
- The reviewed helper uses ten seconds of settling time, a nominal five-second
  prediction window and one-second edge margins.
- Predictions were hidden by the helper until the session ended; the protocol
  also asked the participant to minimize the app's prediction display.
- The transcript does not establish the exact start/end time of each trial.
  The UTC value embedded in the session ID identifies the session name, not a
  substitute for the missing trial timestamps.

## Reported confusion matrix

Rows are the requested actual activity; columns are the predicted activity.

| Actual activity | Predicted WALKING | Predicted RUNNING | Eligible windows |
| --- | ---: | ---: | ---: |
| WALKING | 25 | 1 | 26 |
| RUNNING | 0 | 26 | 26 |
| Total | 25 | 27 | 52 |

| Metric within this session | Checked arithmetic |
| --- | --- |
| Overall window accuracy | 51 / 52 = 98.0769%, rounded to 98.1% |
| WALKING windows identified correctly | 25 / 26 = 96.2% |
| RUNNING windows identified correctly | 26 / 26 = 100.0% |
| Observed classification error in the summary | One WALKING window predicted RUNNING, in trial 1 |

These percentages measure agreement with the participant-labelled timed
intervals in this session. They are not model confidence values.

## Reported trials

| Trial | Requested activity | Participant confirmed | Eligible windows | Correct predictions | Helper quality result |
| --- | --- | --- | ---: | ---: | --- |
| 1 | WALKING | Yes | 9 | 8 | Valid |
| 2 | RUNNING | Yes | 9 | 9 | Valid |
| 3 | WALKING | Yes | 9 | 9 | Valid |
| 4 | RUNNING | Yes | 8 | 8 | Valid |
| 5 | WALKING | Yes | 8 | 8 | Valid |
| 6 | RUNNING | Yes | 9 | 9 | Valid |
| Total | Both classes | All six | 52 | 51 | All six reported Valid |

The helper's COMPLETE result means its configured trial-quality checks passed
according to the supplied output. Independent confirmation of those checks
requires the original session data.

## Data-flow implications

The earlier October 25 attempt stopped because its latest prediction was stale.
The later output completes all six trials. The reviewed helper checks freshness
before each trial, reads SQLite in read-only mode and selects prediction IDs
beyond each trial's baseline. This supports resumed prediction flow during the
movement session.

The summary does not itself capture current board-side TLS/PUBACK logs, the
individual sensor sampling cadence or matching current HTTPS/Android records.
Those are separate observations; exact prediction cadence can be reviewed after
the raw session is available.

## Preserve and review the original files

The helper printed this Ubuntu results directory:

```text
/home/ubuntu/rt-spark-activity-recognition/docs/testing/timed_walk_jog_20261025T153506Z_0abffb
```

Transfer its original `session.json` and `summary.md` into the integration
worktree. Keep the session directory name unchanged. Review and commit those
files as the next evidence milestone; do not recreate the raw JSON from this
aggregate report.

Raw review should recompute the time/ID selection, check the six confirmations
and quality reasons, compare all counts and identify the single incorrect
window. Incorrect and low-confidence predictions must remain included.

## Limits and remaining laboratory checks

This is a short, fixed-order, single-participant test with one hand-held
placement. Windows from the same trials do not establish independent evidence
across people, placements or environments. There is no resting class.

Database timestamps mark prediction completion, not individual sensor
acquisition. Settling time and margins reduce transition ambiguity but do not
measure transport delay. The trial does not evaluate training cross-validation
or held-out generalization.

After preserving and reviewing the raw session, finish the current Android
physical-phone demonstration, capture remaining board acknowledgement evidence,
and complete the CubeMX regeneration review in
[STM32_INTEGRATION.md](../STM32_INTEGRATION.md). Retain or recover the original
model-training/evaluation evidence separately from this movement result.

The earlier exploratory trial remains in
[the October 20 report](2026-10-20_walk_jog_trial.md). Its retrospectively inferred
activity boundaries are not used to score this new timed session.

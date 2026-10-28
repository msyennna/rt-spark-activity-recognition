# STM32CubeMX walking and jogging trial: October 25, 2026

## Result and provenance

The supplied original session data confirms **51/52 correct eligible windows
(98.1%)**, with six valid trials. Independent selection from the raw rows
reproduces every stored trial analysis and the aggregate confusion matrix.
Replaying the reviewed helper regenerates both supplied files byte-for-byte.

Session: `timed_walk_jog_20261025T153506Z_0abffb`.

- [Original session.json](timed_walk_jog_20261025T153506Z_0abffb/session.json)
- [Original summary.md](timed_walk_jog_20261025T153506Z_0abffb/summary.md)
- [Machine-readable audit](timed_walk_jog_20261025T153506Z_0abffb/audit.json)

The reviewer reanalysed the uploaded files locally and did not observe the
participant or operate the board. This verifies consistency of supplied
evidence; it is not a second independent physical trial.

The repository source reviewed for replay is
`ebc5fc92b3f4e9a3ba8ff87d5a7e4a8d574ed62a`, evaluator v1.0,
Git blob `0238eb68d0b997bfc1f7378580b4d5de143ee8c6`.
The local replay copy was checked against that exact blob. The actual VM script,
flashed firmware and model hashes were not supplied.

## Recorded protocol

- One participant; board placement: **held in right hand**.
- Three rounds alternating WALKING and RUNNING: six requested 60-second trials.
- WALKING meant normal walking; RUNNING trials requested jogging.
- Each trial followed a three-second countdown. Breaks were allowed between trials.
- The participant confirmed completing the requested activity in all six intervals.
- Activity order and server-side timestamps were recorded before prediction comparison.
- Ten settling seconds, a nominal five-second prediction window and one-second
  edge margins define the eligible interval.
- The helper hid comparisons until the end and asked the participant to minimize
  the app's prediction display. No independent observation of that action is claimed.

The session was created at `2026-10-25T15:35:06.440728Z` and finished at
`2026-10-25T15:43:29.336782Z`. Those are session metadata; the per-trial movement
boundaries are recorded separately below.

## Verified confusion matrix

Rows are requested actual activity; columns are predicted activity.

| Actual activity | Predicted WALKING | Predicted RUNNING | Eligible windows |
| --- | ---: | ---: | ---: |
| WALKING | 25 | 1 | 26 |
| RUNNING | 0 | 26 | 26 |
| Total | 25 | 27 | 52 |

| Metric within this session | Recomputed result |
| --- | --- |
| Overall window accuracy | 51 / 52 = 98.0769%, rounded to 98.1% |
| WALKING windows identified correctly | 25 / 26 = 96.2% |
| RUNNING windows identified correctly | 26 / 26 = 100.0% |

These percentages measure agreement with participant-labelled timed intervals.
They are distinct from the model's per-window confidence values.

## Verified trial boundaries and selected IDs

All displayed times are **October 25, Asia/Manila (UTC+8)**, to milliseconds.
The original JSON retains full UTC microsecond timestamps.

| Trial | Activity | Start, Manila | End, Manila | Correct/scored | Eligible IDs |
| --- | --- | --- | --- | ---: | --- |
| 1 | WALKING | 23:36:00.857 | 23:37:00.857 | 8/9 | 4087–4095 |
| 2 | RUNNING | 23:37:24.403 | 23:38:24.404 | 9/9 | 4101–4109 |
| 3 | WALKING | 23:38:40.263 | 23:39:40.264 | 9/9 | 4116–4124 |
| 4 | RUNNING | 23:39:54.438 | 23:40:54.438 | 8/8 | 4131–4138 |
| 5 | WALKING | 23:41:08.506 | 23:42:08.507 | 8/8 | 4146–4153 |
| 6 | RUNNING | 23:42:23.335 | 23:43:23.335 | 9/9 | 4160–4168 |

All six requested-activity confirmations and quality checks pass reanalysis.
Monotonic durations range from 60.000157 to 60.000180 seconds. The largest
recorded UTC/monotonic duration difference is under 0.000037 seconds; this
compares recorded clocks and does not independently establish clock accuracy.

## Independent raw-data audit

The originals contain **82 distinct raw prediction records**. Reanalysis selects
**52 unique eligible windows**. The other **30 records remain preserved** and
are excluded by the predefined time/baseline rule.

For each trial, a row is eligible only when both conditions hold:

1. Its prediction ID is greater than that trial's baseline latest ID.
2. Its database timestamp is between trial start + 16 seconds and trial end -
   1 second, inclusively.

Class label and confidence do not affect selection. The audit independently
recomputed those bounds and IDs, then checked:

- JSON structure, finite confidence values, valid labels/class IDs and UTC formats.
- Raw rows within the recorded capture ranges, increasing IDs/timestamps and
  consistent records, with no duplicate selected ID across trials.
- Trial order, confirmations, planned ends, monotonic duration and UTC agreement.
- Per-trial counts, correctness, quality results and the aggregate metrics.
- Agreement with the exact repository helper's record validation, trial analysis,
  aggregation and file regeneration.

Among the **46 adjacent eligible prediction intervals within trials**, 44 are
5 seconds and two are 6 seconds; median 5 seconds, mean 5.0435 seconds.
These are prediction-completion intervals at the database timestamp's one-second
resolution. They do not measure the individual sensor acquisition interval.

## Single scored error retained

- Trial: **1, WALKING**.
- Prediction ID: **4092**.
- Timestamp: **October 25, 23:36:44 Asia/Manila**
  (`2026-10-25 15:36:44` UTC).
- Prediction: **RUNNING**, confidence **50.5%**.
- The incorrect window remains included in the 51/52 result.

All other eligible windows match their requested activity. Raw records outside
eligible intervals are retained without being assigned movement-test correctness.

## Original file integrity

Both uploaded originals are committed unchanged; the hashes below describe the
attached bytes. Exact helper replay produces the same two files.

| File | Bytes | SHA-256 |
| --- | ---: | --- |
| session.json | 21,941 | `2015ac3ce8fd4326c4ed4b1a3982d8354825fdcddf8241f7bd5fa5e86a15c002` |
| summary.md | 1,985 | `7869c10e9363ef43795dfddcecf5f609d8f2ec41a188da41d47a529e7c10315c` |

## Limits and remaining checks

This is one short, fixed-order test with one participant and one hand-held
placement. Its windows do not establish independent performance across people,
placements or environments. There is no resting class.

Database timestamps mark prediction completion, not sensor acquisition.
Settling time and margins reduce transition ambiguity but do not measure
transport delay. This trial does not evaluate training cross-validation or
held-out generalization.

Current board-side acknowledgements, matching HTTPS/Android display records,
the physical Android-phone demonstration and the CubeMX regeneration review
remain separate checks in the [firmware verification guide](../../firmware/README.md#integration-verification).
Original model-training/evaluation evidence should be retained or recovered
separately.

The [October 20 API record extract](2026-10-20_walk_jog_records.json) remains
historical data. Those earlier records are separate from this session's
independently selected scoring intervals.

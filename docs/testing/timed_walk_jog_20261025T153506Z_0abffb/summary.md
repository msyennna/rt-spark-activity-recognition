# Independently timed walking and jogging test

Session: `timed_walk_jog_20261025T153506Z_0abffb`

Status: **COMPLETE**

Window accuracy on valid recorded intervals: 98.1% (51/52 scored windows).

A partial or incomplete session does not fulfill the planned test protocol.

| Actual activity | Predicted WALKING | Predicted RUNNING |
| --- | ---: | ---: |
| WALKING | 25 | 1 |
| RUNNING | 0 | 26 |

## Trials

| Trial | Requested activity | Confirmed by participant | Eligible windows | Matching predictions | Data quality |
| --- | --- | --- | ---: | ---: | --- |
| 1 | WALKING | True | 9 | 8 | Valid |
| 2 | RUNNING | True | 9 | 9 | Valid |
| 3 | WALKING | True | 9 | 9 | Valid |
| 4 | RUNNING | True | 8 | 8 | Valid |
| 5 | WALKING | True | 8 | 8 | Valid |
| 6 | RUNNING | True | 9 | 9 | Valid |

## Conditions and method

- Board placement: held in right hand
- Protocol and requested activity order were fixed before predictions were inspected.
- Start/end UTC times were recorded on the same Ubuntu host that timestamps predictions.
- A monotonic clock controlled duration; UTC/monotonic disagreements flag the interval.
- Predictions were not displayed until all requested trials finished or the session stopped.
- Selection uses time only. Incorrect predictions and low confidence do not cause exclusion.
- Participant-confirmation and data-quality exclusions retain raw rows and reasons in session.json.

## Limits

This is a short, non-randomized, single-participant test. It does not establish accuracy across people or environments.
Database timestamps mark prediction completion, not individual sensor acquisition. Window starts are estimated using the nominal five-second window. Warm-up and edge exclusions reduce transition ambiguity but do not measure sensor/network delay.
Confidence is not accuracy. The model has WALKING/RUNNING labels only. No resting class or training cross-validation is evaluated here.

Machine-readable evidence: [session.json](session.json).

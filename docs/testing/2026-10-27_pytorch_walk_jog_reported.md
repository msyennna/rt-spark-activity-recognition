# Completed PyTorch walking/jogging test: reported results

Reviewed October 27, 2026 (Asia/Manila). This report records aggregate results
from the participant's pasted console summary. Raw terminal output,
infrastructure identifiers and individual prediction rows are excluded.

## Evidence status

The tool reported `COMPLETE`, with all six 60-second trials participant-confirmed
and marked valid. Board placement was held in the right hand. Activity order
was fixed before predictions were inspected, and predictions remained hidden
until the session ended. The freshness preflight passed.

The original `session.json` was subsequently uploaded and independently
replayed. All six saved trial analyses and aggregate metrics match the
reconstruction, confirming the totals below. The follow-up [original-session
audit](2026-10-27_pytorch_walk_jog_audit.md) closes the earlier pending review.
A new model hash was not supplied with this run; attribution to the active
PyTorch deployment follows the preceding deployment history.

## Reported results

| Actual activity | Predicted WALKING | Predicted RUNNING | Correct / eligible | Correct identification |
| --- | ---: | ---: | ---: | ---: |
| WALKING | 26 | 0 | 26 / 26 | 100.0% |
| RUNNING (jogging task) | 24 | 3 | 3 / 27 | 11.1% |

Overall window accuracy: **29 / 53 = 54.7%**.
Balanced accuracy, the mean of the two class recalls: **55.6%**.

| Trial | Requested activity | Eligible windows | Matching predictions | Reported quality |
| --- | --- | ---: | ---: | --- |
| 1 | WALKING | 9 | 9 | Valid |
| 2 | RUNNING | 9 | 0 | Valid |
| 3 | WALKING | 9 | 9 | Valid |
| 4 | RUNNING | 9 | 0 | Valid |
| 5 | WALKING | 8 | 8 | Valid |
| 6 | RUNNING | 9 | 3 | Valid |

All walking windows matched the requested class. Jogging was mostly labelled
Walking; the first two jogging trials had no matching eligible windows.
Protocol completion therefore coexists with weak jogging recognition.

## Interpretation and limits

This is a short, non-randomized, single-participant test with one board
placement. The 53 windows are clustered within six trials; they are not 53
independent participants or recording sessions. The result does not establish
accuracy across people or environments.

The timing tool selects windows by recorded time and coverage, not predicted
class or confidence. Incorrect predictions remain in the totals. Confidence
values were absent from the original console summary; aggregate confidence
findings from the subsequent uploaded file are recorded in the follow-up audit.
Prediction-completion timestamps do not directly measure sensor acquisition
or network delay.

A source-level contract review found that the firmware sends acceleration in
g and converts gyroscope degrees/second to rad/s, matching the PyTorch input
contract and its all-axis feature order. This does not identify the cause of
the poor jogging result or prove what was running during each interval. Raw
labelled sensor observations would be needed to investigate motion, sampling
or training/deployment differences.

The previous 98.1% live result belongs to Random Forest. The PyTorch dataset
holdout result belongs to a separate dataset evaluation. Neither substitutes
for the 54.7% live result reported here. The model has only WALKING/RUNNING
classes; this test supplies no resting-class evaluation.

## Completed follow-up

The original file audit is complete. All six durations, participant
confirmations, eligible windows, quality checks and aggregate metrics match.
Preserve this completed baseline and the earlier stopped attempt. Present
the actual result and limitations in the class submission, retaining incorrect
eligible predictions and the existing scoring rules.

No firmware, model, credentials, thresholds or scoring rules were changed for
this review. No immediate repeat trial is needed merely to replace a weak
score. Any later model adjustment should use separate development data and a
new evaluation, retaining this baseline.

[Machine-readable reported summary](2026-10-27_pytorch_walk_jog_reported.json).

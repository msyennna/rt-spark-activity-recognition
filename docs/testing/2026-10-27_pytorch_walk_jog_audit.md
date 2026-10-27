# Original-session audit: completed PyTorch movement test

Reviewed October 27, 2026 (Asia/Manila), using the participant's uploaded original
`session.json`. This note publishes aggregate findings only. Raw prediction
rows, exact event times and infrastructure identifiers are excluded.

## Independent verification

The uploaded file contains 83 recorded prediction rows across six trials.
Recalculation selected 53 unique eligible windows using the existing start/end,
warm-up, nominal-window, edge-margin and baseline-ID rules. The other 30 rows
remain context outside those predefined selection rules. Selection did not
use predicted class or confidence.

All six participant confirmations, planned durations, clock-consistency checks,
coverage checks, duplicate checks and prediction-cadence checks pass. All saved
trial analyses and aggregate metrics match the independent reconstruction.
Selected prediction intervals range from five to six seconds. The largest
UTC-versus-monotonic duration disagreement is under 0.000037 seconds.

| Trial | Requested activity | Eligible windows | Correct predictions | Independently verified |
| --- | --- | ---: | ---: | --- |
| 1 | WALKING | 9 | 9 | Pass |
| 2 | RUNNING | 9 | 0 | Pass |
| 3 | WALKING | 9 | 9 | Pass |
| 4 | RUNNING | 9 | 0 | Pass |
| 5 | WALKING | 8 | 8 | Pass |
| 6 | RUNNING | 9 | 3 | Pass |

| Actual activity | Predicted WALKING | Predicted RUNNING | Correct identification |
| --- | ---: | ---: | ---: |
| WALKING | 26 | 0 | 100.0% |
| RUNNING (jogging task) | 24 | 3 | 11.1% |

Overall accuracy is **29/53 = 54.7%**. All 24 incorrect eligible predictions
remain scored. The completed protocol and its saved result are now verified;
another six-trial run is not needed merely to validate these counts.

## Confidence and limitations

Incorrect eligible predictions have reported confidence between approximately
87.9% and 99.99%, with a mean of approximately 98.8%. High confidence therefore
does not establish correctness in this live test.

Activity and right-hand placement are participant-reported. The file contains
prediction records, not raw six-axis motion samples or acquisition timestamps.
It does not embed a checkpoint hash. Attribution to the active PyTorch model
follows the preceding deployment evidence; this audit does not invent a new
runtime hash check. The single participant, fixed trial order and clustered
windows limit generalization claims.

The classification result is weak for jogging despite valid timing and
coverage. This is a model-performance limitation to report and, if further
development is pursued, investigate with separate development data and a fresh
evaluation. The older Random Forest score is not a replacement for this result.

## Laboratory completion boundary

The original seven-page handout was re-read for this review. It describes three
core work areas: cloud/model, embedded firmware and Android app. The existing
repository evidence covers their implementation and end-to-end operation;
this upload closes the pending original-session audit.

The handout encourages rigorous testing and good generalization but supplies
no numerical accuracy pass mark, six-trial quota, compulsory repeated
diagnostic protocol, report template or video format. The extra continuity,
fault-recovery and memory checks in the engineering checklist should not
silently become new requirements for completing this handout.

The core implementation and planned live evaluation are complete on the
documented actual-board configuration. Weak jogging recognition remains a
quality limitation, and the hardware/toolchain adaptation remains documented.
Submission and grading follow the class's actual instructions; this audit
does not imply an instructor grade or approval.

No firmware, trained weights, credentials, cloud services, temporal guard or
scoring rules were changed. The participant's original uploaded file is
preserved privately; only this aggregate audit is committed.

[Machine-readable aggregate audit](2026-10-27_pytorch_walk_jog_audit.json).

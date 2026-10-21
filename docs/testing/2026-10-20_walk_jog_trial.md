# Walking and jogging trial — October 20, 2026

## Result and evidence status

The exported records contain a sequence consistent with the participant's
reported 40 seconds of walking followed by 40 seconds of jogging. Under the
candidate eight-window-per-activity allocation below, all 16 predictions match
the reported activity sequence: eight WALKING followed by eight RUNNING.

**This is a provisional activity-agreement summary, not an independently timed
accuracy measurement.** Actual movement start/stop timestamps were not logged.
The candidate allocation uses reported durations, nominal five-second prediction
windows, and the last jogging prediction in the screenshots. Its agreement must
not be presented as proof of 100% model accuracy.

## Participant-reported conditions

- Normal walking for 40 seconds, immediately followed by jogging for 40 seconds.
- The board was kept at approximately the same level throughout the trial.
- The participant reported that the Walking history did not include table rest.
- Device placement, exact orientation, and movement start/stop timestamps were
  not separately logged. Warm-up or transition-window exclusions were not
  independently recorded.
- Jogging is compared with the model's RUNNING label.

The system publishes approximately one sensor reading per second and classifies
non-overlapping windows of five readings. The eighty-second protocol therefore
corresponds to approximately sixteen prediction windows.

## Candidate trial allocation

The candidate trial consists of records **#2419–#2434**. These are the sixteen
consecutive records ending at #2434, the last jogging result visible in the
supplied screenshots. The first eight are provisionally assigned to walking and
the next eight to jogging.

| Reported activity | Candidate record IDs | Matching predictions under this allocation | Mean model confidence | Confidence range |
| --- | --- | --- | --- | --- |
| Walking, 40 seconds | #2419–#2426 | 8 of 8 | 67.5% | 56.0–85.5% |
| Jogging, 40 seconds | #2427–#2434 | 8 of 8 | 88.2% | 71.0–99.0% |

The database timestamps for these sixteen predictions are 15:21:11–15:22:26 UTC
on October 20, 2026, equivalent to 23:21:11–23:22:26 in Asia/Manila on that date.
The 75-second gap from the first recorded prediction to the last does not mean
that all sixteen nominal five-second windows cover only 75 seconds: the first
prediction also summarizes preceding readings. Actual sample-window boundaries
cannot be recovered from these database rows alone.

Records #2418 and #2435 are retained below as adjacent context, outside the
candidate allocation. Their actual physical activities were not independently
logged, so neither is counted as a correct or incorrect prediction.

## Exported database evidence

The participant supplied a read-only query of `activity_records` for inclusive
IDs 2418–2435, ordered by ID. All eighteen IDs are consecutive and every adjacent
database timestamp differs by five seconds. This describes the exported rows;
it does not establish packet-level delivery or end-to-end latency.

Confidence is copied from the console output, which displayed one decimal place
in percentage units. Mean confidence is the arithmetic mean of those displayed
values, not a measured correctness rate.

| Record ID | Database time, UTC | Provisional reported activity | Predicted activity | Model confidence |
| --- | --- | --- | --- | --- |
| 2418 | 15:21:06 | Context only | WALKING | 71.5% |
| 2419 | 15:21:11 | WALKING | WALKING | 73.5% |
| 2420 | 15:21:16 | WALKING | WALKING | 68.5% |
| 2421 | 15:21:21 | WALKING | WALKING | 85.5% |
| 2422 | 15:21:26 | WALKING | WALKING | 75.5% |
| 2423 | 15:21:31 | WALKING | WALKING | 60.5% |
| 2424 | 15:21:36 | WALKING | WALKING | 60.5% |
| 2425 | 15:21:41 | WALKING | WALKING | 60.0% |
| 2426 | 15:21:46 | WALKING | WALKING | 56.0% |
| 2427 | 15:21:51 | RUNNING | RUNNING | 88.5% |
| 2428 | 15:21:56 | RUNNING | RUNNING | 99.0% |
| 2429 | 15:22:01 | RUNNING | RUNNING | 82.5% |
| 2430 | 15:22:06 | RUNNING | RUNNING | 92.5% |
| 2431 | 15:22:11 | RUNNING | RUNNING | 89.5% |
| 2432 | 15:22:16 | RUNNING | RUNNING | 89.0% |
| 2433 | 15:22:21 | RUNNING | RUNNING | 93.5% |
| 2434 | 15:22:26 | RUNNING | RUNNING | 71.0% |
| 2435 | 15:22:31 | Context only | WALKING | 66.0% |

The machine-readable companion file is
[2026-10-20_walk_jog_records.json](2026-10-20_walk_jog_records.json). It includes
all eighteen rows, the participant's reported protocol, the allocation rule,
conditional counts, and the timing limitations.

The earlier app header showed 42 WALKING and 8 RUNNING predictions among its
latest 50 historical records. Those counts cover more history than the reported
80-second protocol and are not used as the trial sample size.

## Interpretation and limits

The observed prediction change is consistent with a real walking-to-jogging
change reported by the participant. The confirmed transport and app behavior,
combined with these stored records, supports a successful exploratory live
movement demonstration.

The allocation is retrospective. The visible class transition helps explain the
sequence, but it cannot independently validate the exact movement boundary.
Choosing boundaries from model outputs can overstate apparent performance.
Consequently, no independently timed accuracy, precision, recall, F1 score, or
formal confusion matrix is claimed for this trial.

This is one short, single-participant trial with temporally related windows.
It does not establish performance across people, mounting positions, movement
speeds, or environments. The model has WALKING and RUNNING classes only; this
trial does not validate resting recognition. Confidence percentages do not
measure generalization accuracy.

## Next evaluation milestone

For a scored validation trial, record activity labels and start/stop timestamps
before examining predictions. Keep board placement consistent, explicitly mark
transition intervals, and repeat walking and jogging trials. Select the evaluation
windows from those independent time labels, rather than from predicted classes.
Then calculate per-class correct/incorrect counts and a confusion matrix. Keep
this exploratory record as evidence instead of replacing it with later results.

## Repository milestone

This milestone adds only this report and its JSON evidence. It does not alter
firmware, the model, the running classifier, the database, or the Android app.
Commit and push the two documentation files after checking their staged names.
The user's standing workflow is to commit and push each completed project
milestone, including documented test results.

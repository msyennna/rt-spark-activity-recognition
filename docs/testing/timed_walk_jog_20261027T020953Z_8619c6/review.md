# Stopped trial audit and later stream recovery

Recorded October 27, 2026 (Asia/Manila), from the complete supplied session
object, classifier status, five subscriber messages and journal excerpts. The
[session transcription](session.transcribed.json) preserves the session object;
calculations and later observations are recorded below. The source is a
transcription rather than an original server-file download. No byte-exact
comparison with the server JSON or original summary is claimed.

## Trial-data audit

| Event | UTC |
| --- | --- |
| Only saved prediction: ID 7170 | 02:10:08 |
| Walking START | 02:10:09.969793 |
| Earliest eligible prediction completion | 02:10:25.969793 |
| Latest eligible prediction completion | 02:11:08.969927 |
| Walking STOP | 02:11:09.969927 |
| Session stopped at next-trial preflight | 02:11:16.799096 |

ID 7170 is newer than baseline ID 7169 but completed **1.969793 seconds before
walking started**. It is outside the scoring interval. The saved data contains
no prediction completions during that walking interval and no eligible windows;
the predeclared minimum was eight. The monotonic duration was 60.00016801094171
seconds and UTC elapsed time was 60.000134 seconds, with about 0.000034 seconds
disagreement, so this record does not indicate a timer/clock-jump exclusion.

Independent time/ID selection agrees with the saved empty eligible-ID list.
The existing helper reproduces the complete analysis and aggregate metrics.
Replaying its session save in a temporary directory reproduces the JSON object
semantically and a STOPPED summary. Original summary-file bytes were not supplied.
The session remains **STOPPED, zero scored windows, no accuracy result**. A
pre-start WALKING prediction must not be counted as a correctly classified walk.

## Later recovery observations

The reported power source during the trial was a **power bank**. The later
service status is active/running with PID 131672 and an active
start time of October 26 at 04:07:58 UTC. No classifier process restart during
the trial is indicated by that status. This does not rule out a temporary
processing error or missing input while the same process remained alive.

The later subscriber receives five six-field finite sensor objects. Journal
entries show consecutive prediction IDs 7429, 7430 and 7431 at **02:33:06,
02:33:11 and 02:33:16 UTC**, two five-second completion gaps. IDs 7430 and 7431
are WALKING with displayed confidences 99.89% and 99.88%. Receipt messages show
two complete five-reading windows at one-second displayed timestamp steps.
This establishes later reception/prediction, not coverage during the earlier
trial, precise acquisition timing, movement accuracy or a proven repair.

## Root cause remains open

The currently supplied journal excerpt covers 02:33 UTC. It cannot determine
whether the failed 02:10–02:11 interval involved missing MQTT input, a processing
error, a power transition, Wi-Fi loss or another interruption. Power-bank use
alone does not prove that the bank shut off. No model, classifier, firmware or
quality-rule change has been made.

In Ubuntu, inspect the **failed interval** rather than only the latest lines:

```bash
journalctl -u rtspark-classifier.service --utc --since "2026-10-27 02:09:45 UTC" --until "2026-10-27 02:11:30 UTC" --no-pager
```

Record whether the board was already running on the power bank before the
countdown or changed power at the start, plus any serial/reset/LED observation.
Then verify continuous reception under the intended power/placement arrangement
before a new movement session. Preserve this stopped attempt. A future repeat
must create a new session and retain incorrect predictions and predeclared
exclusions; it cannot turn this incomplete attempt into a model-accuracy result.

## Follow-up: requested trial-time journal supplied

The subsequent [delivery review](delivery_review.md) records six explicit gap
resets, including a 44-second displayed receipt gap. Prediction 7171 appears
79 seconds after 7170, after this session stopped. This identifies repeated
partial-window resets as the immediate cause; the underlying power/link or
transport cause remains unproven. The next step is a short timestamped receipt
check with the intended power setup already connected.

## Retained audit details

The session transcription has SHA-256
`239ae0c0fa407b2a98153e51e452602537fe8d0f518946c0cc98416aa7419648`.
Its elapsed-time calculations agree with the helper replay; absolute UTC versus
monotonic disagreement is `3.401094170385477e-05` seconds.
The eligible-ID set is empty, aggregate accuracy is null, and the replayed
summary is STOPPED. Original server files were left unchanged during the audit;
no firmware, model, classifier or quality-rule changes were made.

| Later classifier status | Value |
| --- | --- |
| State / PID | active (running) / 131672 |
| Active since | 2026-10-26 04:07:58 UTC |
| Current / peak memory | 154.2 / 172.3 MiB |
| Executable | `/home/ubuntu/rt-spark-activity-recognition/.venv/bin/python -u cloud/mqtt/mqtt_classifier.py` |

The later five subscriber readings are preserved below. These are later
recovery observations, not sensor samples from the stopped scoring interval.

| Reading | acceleration_x | acceleration_y | acceleration_z | gyro_x | gyro_y | gyro_z |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | -0.006897 | 0.009521 | 1.010559 | -0.052493 | 0.002531 | -0.002798 |
| 2 | -0.005005 | 0.01062 | 1.010315 | -0.053825 | 0.001599 | -0.003064 |
| 3 | -0.006836 | 0.008911 | 1.01178 | -0.054492 | 0.002531 | -0.002665 |
| 4 | -0.005371 | 0.010254 | 1.011719 | -0.053426 | 0.002398 | -0.001732 |
| 5 | -0.005493 | 0.008118 | 1.01123 | -0.053559 | 0.001332 | -0.002931 |

The [trial-time journal transcription](journal.transcribed.json) separately
records six explicit gap resets, a maximum displayed receipt gap of 44 seconds,
and a 79-second gap from prediction 7170 to 7171. The immediate cause was
repeated partial-window resets; the underlying source of delivery gaps remains
undetermined. The diagnostic interval was 02:09:45–02:11:30 UTC. These stopped
attempt observations do not alter the later completed six-trial result.

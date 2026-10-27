# Stopped PyTorch walking/jogging attempt

Reviewed October 27, 2026 (Asia/Manila). The user supplied the model hash and
terminal summary for `timed_walk_jog_20261027T020953Z_8619c6`. This report
preserves those reported facts; the original `session.json` and `summary.md`
have not yet been supplied for raw audit. See
[the structured report](2026-10-27_pytorch_trial_stopped.json).

## Reported outcome

**STOPPED; no accuracy score is available.** The on-disk model hash printed
immediately before the timer matches the deployed PyTorch checkpoint:
`be60d75d699a6bc317806426c2ec7e47432c2e907e0ceafbab145ba16cce3331`.
That verifies file identity, not current classifier-process health.

The participant recorded placement as **held in right hand**, completed the
first 60-second WALKING interval and answered `y`. Its summary reports zero
eligible windows and the quality issue:

> Prediction count does not cover the expected interval at the nominal five-second rate.

At the readiness prompt for trial 2 (RUNNING), preflight stopped with:

```text
ValueError: Latest prediction is older than 15 seconds. Power the board and wait for fresh predictions.
```

No second START cue or jogging interval is shown. The requested six-trial
protocol is incomplete, both class supports are zero, and the zero-valued
confusion matrix is **not a 0% accuracy result**. It also does not establish
that no raw rows existed outside the eligible time window. Timing/row detail
requires the original session data.

The source checks freshness before trial 1 and again before later countdowns.
Reaching the first START cue means that preflight accepted a recent record at
that point; reaching the second readiness prompt does not establish continued
sensor delivery. The supplied output indicates lost prediction coverage and
subsequent staleness, not a demonstrated classifier misclassification.

Original server results folder (retain it):

```text
/home/ubuntu/rt-spark-activity-recognition/docs/testing/timed_walk_jog_20261027T020953Z_8619c6
```

## Diagnostic next step

Determine where the stream stopped before repeating the movement protocol.
Possible power/Wi-Fi loss, MQTT interruption, and classifier failure remain
hypotheses; none is established by this summary. The classifier deliberately
clears its partial temporal buffer after delivery gaps over 2.5 seconds, so
irregular readings may fail to produce full prediction windows even if a few
messages still arrive. Check actual receipt and service logs.

In the existing Ubuntu session, read the current state and original trial data:

```bash
systemctl status rtspark-classifier.service --no-pager -l
mosquitto_sub -h 127.0.0.1 -p 1883 -t activity/sensor -C 5 -W 15 -v
journalctl -u rtspark-classifier.service -n 30 --no-pager
cat /home/ubuntu/rt-spark-activity-recognition/docs/testing/timed_walk_jog_20261027T020953Z_8619c6/session.json
```

Also record whether the board remained connected to the laptop, changed to a
power bank, or lost power during movement, and retain any serial error output
around the final PUBACK. These inspections do not restart services, publish
synthetic readings, alter the database, or overwrite the stopped session.

No source fix or repeat trial is claimed here. Keep the predeclared quality
rules and all incorrect/low-confidence predictions. Once the data stream is
stable, a repeat creates a new unique results folder, leaving this attempt
intact. The earlier Random Forest score remains historical evidence.

## Subsequent session-data review

The user later supplied complete session JSON and recovery logs. The
[follow-up audit](timed_walk_jog_20261027T020953Z_8619c6/review.md) reproduces the
saved analysis/metrics: only record 7170 was saved, about two seconds before
walking START, with zero eligible windows. Later reception/prediction is
working at 02:33 UTC; the trial-time cause remains undetermined. The separate
transcription is not an original server-file download.

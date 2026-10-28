# Broker receipt continuity: interruptions reproduced

Reviewed October 27, 2026 (Asia/Manila). This is a connection diagnostic,
not a scored walking/jogging trial. It follows the
[trial-time delivery-gap review](../timed_walk_jog_20261027T020953Z_8619c6/delivery_review.md).

## Source and method

The user ran this command in the Ubuntu SSH terminal and pasted its output:

```bash
mosquitto_sub -h 127.0.0.1 -p 1883 -t activity/sensor -R -F '%I %U' -C 70 -W 90
```

[receipt_times.transcribed.txt](receipt_times.transcribed.txt) preserves the
61 timestamp lines and final `Timed out` message. Login/banner/prompt text is
omitted. This is a transcription, not an independently downloaded server file.
[analysis.json](analysis.json) records its SHA-256, every receipt and interval,
the calculation results, provenance and limits. Fractional Unix seconds were
subtracted with decimal arithmetic to retain the printed precision; their
integer seconds agree with each corresponding ISO timestamp.

The preceding instructions requested a power bank connected before readiness,
a location near the same Wi-Fi access point and about one minute of walking.
The user previously reported using a power bank. This output does not separately
confirm the power-transition timing, location or actual movement, so those
conditions are not asserted as independently verified.

## Results

| Observation | Result |
| --- | ---: |
| Requested messages | 70 |
| Supplied receipt lines | 61 |
| Final client output | Timed out |
| First-to-last receipt span | 88.117257356 seconds |
| Interarrival intervals | 60 |
| Minimum interval | 0.989012350 seconds |
| Median interval | 1.001077220 seconds |
| Maximum interval | 16.020381773 seconds |
| Intervals over 2.5 seconds | 3 |
| Intervals over 1.5 through 2.5 seconds | 9 |

The capture runs from **03:26:35 to 03:28:03 UTC**, or **11:26:35 to
11:28:03 AM Asia/Manila**. The first-to-last span is not a measurement of
the complete command runtime; the exact timeout instant is not printed.

| Previous receipt UTC | Next receipt UTC | Interval |
| --- | --- | ---: |
| 03:27:14 | 03:27:17 | 2.997996077 seconds |
| 03:27:17 | 03:27:20 | 3.002044956 seconds |
| 03:27:34 | 03:27:50 | 16.020381773 seconds |

**Continuity result: NOT STEADY.** Most intervals are close to one second,
but the three longer interruptions exceed the classifier's existing
2.5-second temporal-gap threshold. A classifier seeing these same gaps would
discard partial windows. No simultaneous classifier journal was supplied for
this capture, so actual reset events during this period are not claimed.

## What this establishes

The separate local subscriber also sees an irregular stream. The observed
symptom therefore extends beyond the classifier's buffer logic. This does not
identify a power-bank fault, a loose cable, Wi-Fi loss, a publisher pause,
TCP/TLS reconnection, or broker/client scheduling as the underlying cause.
No new firmware serial or broker connection log accompanies these timestamps.

These are subscriber receipt times from the server wall clock, not sensor
acquisition timestamps or monotonic clock measurements. The payload has no
acquisition sequence in this capture, and receipt count alone cannot establish
an exact packet-loss count. No duplicate samples, lost-packet percentage,
model accuracy, or timing of the user's movement can be inferred from it.

## Next controlled check

Keep the same power bank, seated cable and Wi-Fi. Put the board **motionless
near the router**, allow it to reach LIVE, and repeat the identical command
above in Ubuntu. Share all timestamp lines and the final message. This is a
stationary baseline; retain both checks even if one fails.

A stationary success would support comparison with movement under those
conditions, not prove the power bank is fault-free or the problem repaired.
If it also has long gaps, obtain correlated publisher serial/connection
diagnostics or a controlled change to the power source before attempting
the full protocol. A single successful message count is insufficient: inspect
the intervals and classifier freshness as well.

The six-trial PyTorch evaluation remains pending. Keep its fixed protocol and
the classifier gap guard unchanged. No firmware, model, service, Android,
credentials or scoring changes were made for this review.

## Follow-up stationary result

The requested [later stationary-baseline capture](../2026-10-27_stationary_continuity/review.md)
is now supplied: all 70 messages arrived, all 69 receipt intervals are between
0.976881122 and 1.028009503 seconds, and no timeout/error is shown. This passes
the short receipt check. The earlier interrupted capture remains valid
evidence; the physical cause is not identified. A short walking comparison
near the router with the same power connection precedes the full protocol.

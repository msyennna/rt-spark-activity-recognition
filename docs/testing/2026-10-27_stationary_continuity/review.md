# Stationary receipt continuity: short capture passed

Reviewed October 27, 2026 (Asia/Manila). Following the
[interrupted subscriber capture](../2026-10-27_broker_continuity/review.md),
the user supplied another run of the same Ubuntu command:

```bash
mosquitto_sub -h 127.0.0.1 -p 1883 -t activity/sensor -R -F '%I %U' -C 70 -W 90
```

## Evidence and calculation

[receipt_times.transcribed.txt](receipt_times.transcribed.txt) contains all
70 pasted timestamp lines. Login/banner/prompt text is omitted; the supplied
output returns to the shell prompt without a timeout/error message. A numeric
exit status was not supplied. This is a transcription, not an independently
retrieved original server file. [analysis.json](analysis.json) records its
SHA-256, every receipt and interval, provenance and limits.

Fractional Unix seconds were subtracted using decimal arithmetic. The integer
seconds agree with the adjacent ISO timestamps. Each timestamp pair represents
one receipt, even when consecutive ISO displays repeat or skip a whole second;
their fractional Unix times show the actual printed separation.

| Measurement | Result |
| --- | ---: |
| Requested / supplied messages | 70 / 70 |
| Interarrival intervals | 69 |
| First-to-last receipt span | 69.096153875 seconds |
| Minimum interval | 0.976881122 seconds |
| Median interval | 1.000012837 seconds |
| Maximum interval | 1.028009503 seconds |
| Intervals within 0.95–1.05 seconds | 69 / 69 |
| Intervals over 2.5 seconds | 0 |

The capture covers **04:10:30–04:11:40 UTC**, or **12:10:30–12:11:40 PM
Asia/Manila**. The first-to-last span does not include command startup or
the unprinted final exit instant.

**Result: PASS for this short subscriber receipt capture.** All messages
arrived with approximately one-second separation and none exceeded the
classifier's existing 2.5-second gap threshold.

## Comparison and limits

| Capture | Supplied messages | Final output | Maximum receipt interval | Intervals over 2.5 s |
| --- | ---: | --- | ---: | ---: |
| Earlier 03:26:35–03:28:03 UTC | 61 / 70 | Timed out | 16.020381773 s | 3 |
| Later 04:10:30–04:11:40 UTC | 70 / 70 | Return to prompt; no error shown | 1.028009503 s | 0 |

The later run followed instructions to keep the same power bank and Wi-Fi
while the board stayed motionless near the router after LIVE readiness.
Power-bank use was previously reported. The user did not separately reconfirm
each physical condition, and the reviewer did not observe them. The captures
also occur at different times. They therefore support a steady later baseline,
not a causal conclusion about movement, router distance, cable seating or
the power bank, and do not establish that the earlier fault is repaired.

These are subscriber receipt timestamps from the server wall clock. They do
not measure sensor acquisition times, identify the publisher process, supply
sample sequence numbers, prove exact loss/duplicate counts or establish
long-running reliability. No simultaneous classifier journal was supplied
for this capture. It is a connection check, not an activity-accuracy test.

## Next step

Keep the same power bank, Wi-Fi and seated cable. Repeat the identical command
while walking in a small area near the router, so movement can be checked
before spending another full six-trial session. Inspect all receipt intervals
and classifier freshness before starting the unchanged timed protocol. If
the interruptions return, preserve that output and obtain correlated board
serial/connection diagnostics rather than treating stationary success as a fix.

The stopped PyTorch session remains preserved and unscorable. The complete
six-trial PyTorch evaluation remains pending. No firmware, classifier,
model, service, Android, credentials, temporal guard or scoring rules changed.

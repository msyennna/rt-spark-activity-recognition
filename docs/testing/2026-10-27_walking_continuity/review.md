# Walking receipt capture: count complete, one long gap

Reviewed October 27, 2026 (Asia/Manila). After the
[short stationary capture](../2026-10-27_stationary_continuity/review.md),
the user supplied another run of the identical Ubuntu subscriber command:

```bash
mosquitto_sub -h 127.0.0.1 -p 1883 -t activity/sensor -R -F '%I %U' -C 70 -W 90
```

## Evidence and results

[receipt_times.transcribed.txt](receipt_times.transcribed.txt) preserves all
70 pasted timestamp lines. Command/prompt text is omitted; the supplied output
returns to the shell prompt with no timeout/error shown. No numeric exit status
was supplied. This is a transcription, not a downloaded original server file.
[analysis.json](analysis.json) retains the transcription SHA-256, every receipt
and interval, calculation results, provenance, conditions and limits.

Decimal subtraction of fractional Unix seconds gives:

| Measurement | Result |
| --- | ---: |
| Requested / supplied messages | 70 / 70 |
| First-to-last receipt span | 72.341852470 seconds |
| Interarrival intervals | 69 |
| Minimum interval | 0.988659792 seconds |
| Median interval | 1.001083763 seconds |
| Maximum interval | 3.006398995 seconds |
| Intervals at most 1.5 seconds | 67 |
| Intervals over 1.5 through 2.5 seconds | 1 |
| Intervals over 2.5 seconds | 1 |

The capture spans **04:19:25–04:20:37 UTC**, or **12:19:25–12:20:37 PM
Asia/Manila**. Its complete command runtime and exit instant are not printed.
The single interval above 2.5 seconds is between receipts 36 and 37:
**04:20:01.589865449–04:20:04.596264444 UTC**, or approximately
**12:20:01–12:20:04 PM Asia/Manila**. Its length is **3.006398995 seconds**.
The other longer interval is **2.003224321 seconds** between receipts 7 and 8.

**Result: message count complete, with one interval over the existing temporal
gap threshold.** The later run is much less interrupted than the first
61-message timeout capture, but it does not reproduce the uniformly short
intervals of the 70-message stationary capture. Reaching the requested count
alone does not establish uninterrupted one-second receipt.

## Interpretation and limits

The preceding instructions requested about one minute of walking in a small
area near the router, with the same power bank, Wi-Fi and securely seated cable.
Power-bank use was previously reported. The user did not separately reconfirm
actual movement start/end or every physical condition, and the reviewer did
not observe them. The captures occur at different times. This comparison does
not identify movement, router distance, power, cable, publisher behavior or
transport as the cause of the interruption.

These are subscriber wall-clock receipt times, not sensor acquisition times,
sequence-numbered samples or a monotonic capture. An exact lost-packet count
cannot be derived. A classifier receiving the same long gap would apply its
2.5-second buffer guard, but no simultaneous classifier log was supplied with
this capture. An actual reset or delayed database record in this period is
therefore not yet established. No accuracy score or completed six-trial
PyTorch evaluation follows from this connection check.

## Next step: inspect the same time period

Read the existing classifier and broker service journal in Ubuntu:

```bash
journalctl -u rtspark-classifier.service -u mosquitto.service --utc --since "2026-10-27 04:19:20 UTC" --until "2026-10-27 04:20:45 UTC" --no-pager
```

This identifies any logged buffer resets, prediction completion gaps and
service/connection events during the receipt capture. Mosquitto can write
connection events to a separate log file; missing journal connection entries
alone do not prove uninterrupted connectivity. Obtain correlated board
serial/transport evidence if these logs do not identify the source of pauses.

Preserve all three receipt captures and the stopped timed session. The full
PyTorch protocol remains pending. No firmware, model, classifier, service,
Android, credentials, temporal guard or scoring rules changed for this review.

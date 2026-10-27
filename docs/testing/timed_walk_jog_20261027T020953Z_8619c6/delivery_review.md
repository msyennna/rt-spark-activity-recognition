# Trial-time delivery gaps: immediate cause identified

Reviewed October 27, 2026 (Asia/Manila). The user supplied the requested
classifier journal for **02:09:45–02:11:30 UTC**. Its receipt, reset and
prediction events are normalized in [journal.transcribed.json](journal.transcribed.json).
This is a transcription of the pasted output, not an independently retrieved
or byte-exact journal export. It supplements the [session audit](review.md).

## What the journal establishes

The classifier produced WALKING records 7166–7170 before the timed interval.
After record 7170 at **02:10:08**, partial buffers repeatedly reached four
readings and then reset because delivery paused. Six explicit resets appear:

| Previous receipt UTC | Reset receipt UTC | Displayed gap | Buffer before reset |
| --- | --- | ---: | ---: |
| 02:10:13 | 02:10:16 | 3 seconds | 4/5 |
| 02:10:19 | 02:10:22 | 3 seconds | 4/5 |
| 02:10:22 | 02:11:06 | 44 seconds | 1/5 |
| 02:11:10 | 02:11:13 | 3 seconds | 4/5 |
| 02:11:14 | 02:11:18 | 4 seconds | 2/5 |
| 02:11:18 | 02:11:23 | 5 seconds | 1/5 |

The displayed timestamps have one-second resolution. The explicit reset
messages separately confirm that the source's monotonic **2.5-second** gap
threshold was exceeded; exact fractional interarrival times were not printed.
There are 46 receipt entries in the supplied interval. They are receipt events,
not sequence-numbered sensor samples or proof of an exact number of lost packets.

The next full window completed as record **7171 at 02:11:27**, **79 seconds**
after record 7170. This is after the walking STOP at 02:11:09.969927 and after
the session stopped at 02:11:16.799096. The journal thus agrees with the saved
trial's empty eligible-prediction set. It shows partial sensor delivery during
the trial, rather than a complete absence of all incoming readings.

## Diagnosis and scope

The immediate failure is **irregular sensor delivery to the classifier**, which
prevents a five-reading temporal window from completing. The gap guard behaved
as designed. No model inference exception or service restart is shown in this
interval. The model, nominal sample interval, window size, freshness threshold
and scoring rules remain unchanged.

The underlying reason for the irregular delivery is still unproven. A power
bank was reported, but these server logs do not identify a power fault, Wi-Fi
signal loss, publisher failure or broker/client transport issue. Later 02:33
logs show recovered reception and five-second predictions; they do not prove
a repair or stable movement operation.

## Short continuity check before a new six-trial session

Prepare the intended power setup **before** starting: connect the power bank,
allow Wi-Fi/MQTT to settle, and wait for LIVE. Use a location close to the same
Wi-Fi access point and keep the USB connection seated. These are controlled
check conditions, not a claim about which factor caused the earlier failure.
Minimize the prediction view after readiness so the receipt check shows no labels.

In Ubuntu run:

```bash
mosquitto_sub -h 127.0.0.1 -p 1883 -t activity/sensor -R -F '%I %U' -C 70 -W 90
```

When timestamp lines begin, walk normally for about one minute while keeping
the same power connection. This is a connectivity check, not a scored activity
trial. The client prints an ISO timestamp and a fractional Unix receipt time
for each fresh message, and exits after 70 messages or its 90-second processing
timeout. Both formats and the count/timeout/retained-message options were checked
in the official [Mosquitto 2.0.18 manual source](https://github.com/eclipse-mosquitto/mosquitto/blob/v2.0.18/man/mosquitto_sub.1.xml)
and [current manual](https://mosquitto.org/man/mosquitto_sub-1.html).

Return the timestamp output and any timeout/error. Receiving 70 messages alone
does not establish continuous one-second delivery: inspect gaps as well. If
broker timestamps are regular but the classifier still resets during the same
period, inspect its local subscription path. If broker receipts also pause,
focus on publisher/link/power evidence, using serial logs or a controlled power
comparison. Do not relax the guard to score data across long interruptions.
A complete repeat of the fixed six-trial protocol follows only after continuity
is established, and the stopped session remains preserved.

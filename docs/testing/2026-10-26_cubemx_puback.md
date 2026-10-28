# CubeMX publisher: accepted MQTT PUBACK capture, October 26, 2026

## Evidence

The user supplied a Windows PowerShell/miniterm transcript from the physical
RT-Spark board, opened on **COM3 at 115200 baud, 8N1**. The excerpt is recorded
on October 26 in Asia/Manila. The transcript itself has no per-line timestamps.

The supplied text is retained in
[`2026-10-26_cubemx_puback.txt`](2026-10-26_cubemx_puback.txt). It contains
sensor JSON, lwIP transmit/receive diagnostics and accepted PUBACK log entries.
No Wi-Fi password, MQTT password, private key or private header is included.

## Checked observations

| Item | Observation |
| --- | --- |
| Accepted PUBACK entries | 17 |
| Packet ID range | 1482 through 1498 inclusive |
| Delivery-counter range | 1482 through 1498 inclusive |
| Sequence | Each visible packet ID and counter increases by one; no gap or duplicate within this excerpt |
| Complete visible reading/ACK pairs | 17 |
| Sensor JSON rows | 18; all six expected fields are numeric and finite |
| Excerpt tail | One additional reading and TX diagnostic; no PUBACK for it is shown |
| Per-line timing | Not present; one-second acquisition/publishing cadence is not measured by this text |

The final partial cycle is retained. An excerpt ending before its next ACK is
not evidence that the reading was lost. The earlier 1,481 deliveries are not
independently established by this excerpt; the counters are recorded as printed.

## Why this supports a QoS 1 acknowledgement pass

At the reviewed integration source commit
`428ab78ce250fd4326f8acd95f4b87fe04d57c29`:

- [rtspark_mqtt_packet.c](../../firmware/src/rtspark_mqtt_packet.c) constructs
  PUBLISH with header 0x32: QoS 1, DUP false and RETAIN false.
- Its `rtspark_packet_puback` parser accepts only a PUBACK header 0x40,
  remaining length 2 and the expected two-byte packet ID.
- [rtspark_mqtt.c](../../firmware/src/rtspark_mqtt.c) checks that parser against
  the pending publish's packet ID before incrementing the delivery counter and
  printing the observed `MQTT PUBACK id=... count=...` line. A mismatch takes
  the failure path.

The transcript is consistent with 17 successfully accepted, matching
acknowledgements in this implementation. The source-to-runtime interpretation
relies on the reviewed implementation; the exact flashed image hash and a
separate decrypted packet capture were not supplied.

**Accepted PUBACK capture: PASS.** This closes the previously missing runtime
acknowledgement evidence. It does not measure the publish interval, prove
global exactly-once processing, repeat TLS certificate tests or exercise a
controlled network outage.

## Downstream evidence and scope

Broker acknowledgement is separate from classifier processing. The
[physical-phone observations](2026-10-26_android_history.md#physical-phone-verification) record phone
LIVE prediction 5167 matching the supplied server API by ID, class, confidence
and UTC-to-Manila time conversion. The [movement-trial report](2026-10-25_cubemx_walk_jog_trial.md)
preserves the audited six-trial result, 51/52 correct eligible windows (98.1%).

This update changes evidence/documentation only. Measured sensor cadence,
extended recovery tests and actual CubeMX regeneration retain their separate
status in the [firmware verification guide](../../firmware/README.md#integration-verification). The
[lab completion review](../LAB_COMPLETION.md) distinguishes functional success
from unresolved handout alignment and reproducibility.

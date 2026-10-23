# STM32CubeMX live MQTT and local API observations

On October 23, 2026, Yena supplied physical-board serial output and Ubuntu shell
results from the integration workflow. The board reported a Wi-Fi connection,
a DHCP lease, verified TLS 1.2 and accepted MQTT login. A server-local MQTT
subscriber received five sensor messages, and the local API returned record 3136.
These observations establish an initial live connection and current local
prediction output; sustained delivery and Android validation remain open.

## Evidence context

- Reviewed integration source baseline: `63842ccd303afddafc055124774893f2e797d1da`.
- The exact flashed binary hash and full programming transcript were not captured.
- Evidence was provided by the user through serial-monitor and Ubuntu shell
  output. The reviewer did not operate the board or independently rerun these
  physical checks.
- The API record's timestamp is a database timestamp, not a measured start time
  for all of the checks below.
- Wi-Fi credentials, SSID, device MAC, local IP addresses, private keys and
  compiled firmware are omitted from this report.

## Observed results

| Check | Observation | Scope |
| --- | --- | --- |
| Sensor startup | `WHO_AM_I: 0xAF`; `ICM20608 INITIALIZED SUCCESSFULLY!`; six numeric fields printed | Sensor startup and readout observed; axis-motion and sample-rate measurements not supplied |
| Wi-Fi and DHCP | `RW007 WIFI CONNECTED`; `LWIP: LINK UP`; `LWIP: DHCP BOUND`; IP/netmask/gateway printed | Initial association and address assignment observed; renewal and reconnect remain untested |
| Transport readiness | `[RTSpark] DNS/TCP connected; UTC ready` | Positive readiness reported; clock drift and RNG fault tests not supplied |
| Board TLS | `[RTSpark] TLS verified: TLSv1.2` | Successful handshake reported by the board; native negative cases remain separate from board-runtime evidence |
| MQTT login | `[RTSpark] MQTT CONNACK accepted; topic activity/sensor QoS1` | Broker accepted login; this line alone does not establish acknowledged publications |
| Broker reception | `mosquitto_sub` returned five six-field readings on `activity/sensor` | Live reception observed; board-side matching PUBACK and timestamped cadence not supplied |
| Local prediction API | Record 3136, WALKING, confidence 0.66, created_at `2026-10-23 06:31:47` UTC | One current SQLite-backed API result observed; advancing records and history still require confirmation |

## Broker reception

The user ran this on Ubuntu:

```bash
mosquitto_sub -h 127.0.0.1 -p 1883 -t activity/sensor -C 5 -W 15 -v
```

The command returned five messages and exited to the shell. One received message:

```json
{"acceleration_x":-0.014465,"acceleration_y":-0.003418,"acceleration_z":1.010376,"gyro_x":-0.051827,"gyro_y":0.004397,"gyro_z":-0.001466}
```

The payload contains the six fields expected by the unchanged classifier.
No sample timestamps are present, so this output does not measure a one-second
sampling interval or link individual samples to a particular prediction window.

## Local API observation

The user then ran:

```bash
curl -sS --max-time 10 http://127.0.0.1:5000/api/activity/latest
```

Response:

```json
{"activity":"WALKING","activity_id":0,"confidence":0.66,"created_at":"2026-10-23 06:31:47","id":3136}
```

This request used loopback HTTP on the server. It does not establish current
external HTTPS access from the new client network or successful Android display.
The earlier workstation timeouts remain historical observations in
[the cloud service report](2026-10-23_cloud_service_check.md).

## Remaining verification

1. Capture board-side PUBACK lines and measure publication/prediction cadence.
2. Read latest predictions twice and inspect history while the board continues
   publishing; record increasing IDs and UTC timestamps.
3. Rerun the CA-verified public service checker from the current Windows network.
4. Confirm matching latest/history records on a physical Android phone and
   observe WAITING FOR SENSOR followed by LIVE after power removal/recovery.
5. Complete the CubeMX regeneration review and remaining physical regression
   checks in [the integration checklist](../STM32_INTEGRATION.md).
6. Run independently timed, labelled walking/jogging trials and retain training
   and evaluation evidence. The 0.66 confidence above is not measured accuracy;
   the actual activity during this connectivity check was not labelled.

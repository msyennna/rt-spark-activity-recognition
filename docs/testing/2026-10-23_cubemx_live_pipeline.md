# STM32CubeMX live pipeline observations and user confirmations

On October 23, 2026, Yena supplied physical-board serial output and Ubuntu shell
results from the integration workflow. The board reported a Wi-Fi connection,
a DHCP lease, verified TLS 1.2 and accepted MQTT login. A server-local MQTT
subscriber received five sensor messages, and the local API returned record 3136.
These supplied logs establish an initial live connection and current local
prediction output. The user later confirmed public connectivity and Android
live/history/recovery checks; the evidence level for that follow-up is recorded
separately below.

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
| Local prediction API | Record 3136, WALKING, confidence 0.66, created_at `2026-10-23 06:31:47` UTC | One current SQLite-backed API result supplied; later live refresh/history confirmation is described below |

The last row describes the supplied shell output. Later user confirmation of
live refresh/history is recorded below; no additional IDs were supplied.

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

## Subsequent user confirmations on October 23

After updating the EC2 inbound source-IP rule for the new network, the user
reported that `tools/validation/check_cloud_services.py` passed. The refreshed
JSON output and its run time were not supplied. This records a reported pass,
not an independently repeated public HTTPS measurement.

Android Studio then reported that `AdbServerController` was stopping/stopped.
After the ADB/emulator recovery instructions, the user said it was working.
The user subsequently confirmed that the requested LIVE refresh, newest-first
history and board-power-removal/recovery checks worked. These are user-reported
outcomes; no new IDs, measured stale interval or screen recording accompanied
the confirmation. The visible Android target was an emulator. A physical-phone
test remains unconfirmed.

Power-cycle recovery does not separately establish DHCP lease renewal or
recovery from a controlled Wi-Fi outage. Board-side packet-ID PUBACK capture
and measured publication cadence also remain open.

## Timed evaluation preparation

The [walking/jogging helper](TIMED_VALIDATION.md) is included for the next
physical test. It records requested labels and UTC intervals on the database
host, reads SQLite in read-only mode, and retains incorrect predictions.
Eight offline tests passed during this update. They use temporary data and
simulated clocks; their deliberately synthetic scores do not measure this model.
No new timed physical trial or model accuracy score is claimed here.

## Remaining verification

1. Capture board-side PUBACK lines and measure publication/prediction cadence.
2. Retain increasing prediction IDs and UTC timestamps from latest/history while
   the board publishes; user confirmation already covers live refresh/history.
3. Retain the next public checker output when validating a network change;
   the current-network pass is recorded above as user-reported.
4. Confirm matching latest/history records on a physical Android phone. Emulator
   LIVE/history and power-cycle recovery have already been confirmed by the user.
5. Complete the CubeMX regeneration review and remaining physical regression
   checks in [the integration checklist](../STM32_INTEGRATION.md).
6. Run the independently timed, labelled [walking/jogging protocol](TIMED_VALIDATION.md)
   and retain training and evaluation evidence. The 0.66 confidence above is not
   measured accuracy; the activity during that connectivity check was not labelled.


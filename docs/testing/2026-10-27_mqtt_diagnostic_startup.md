# Diagnostic firmware: startup and first acknowledgements

Reviewed October 27, 2026 (Asia/Manila), from a user-pasted serial excerpt
following the verified local diagnostic-firmware upload. This note retains
aggregate observations; raw sensor data, addresses and device identifiers
are excluded.

## Observations

- Wi-Fi connection is reported, followed by link-up and DHCP start.
- The sensor identity and initialization checks succeed, and six-field sensor
  readings continue.
- One skipped-offer diagnostic reports `WAIT_READY`, before DHCP is bound.
- DHCP subsequently binds; DNS/TCP connection and UTC readiness are reported.
- TLSv1.2 verification succeeds and MQTT CONNACK is accepted.
- Seventeen consecutive PUBACK packet IDs/counters, from 1 through 17, appear.
- No failure or skipped-offer report after MQTT login appears in the supplied
  excerpt. It ends partway through the following sensor line.

The startup `WAIT_READY` notice is consistent with the firmware waiting for
network/time/random readiness before offering samples to MQTT. In this excerpt
it precedes the IP lease and successful connection sequence. It is not evidence
of an established-session acknowledgement stall.

## Scope and next observation

The new diagnostic message is visible after the reported upload, and the
positive connection/publish path is observed. There are no serial timestamps,
so the excerpt does not establish an exact publish cadence or a full 90-second
observation interval. PUBACKs do not independently establish classifier,
database or Android updates. No underlying interruption or repair is identified.

Keep the current diagnostic session running for another observation interval.
After MQTT login, retain the first failure or skipped-offer message with nearby
PUBACK/network lines. If none appears after about 90 seconds, provide only the
last 20 lines. The startup sequence need not be repeated for this check.

The [diagnostic update](2026-10-27_mqtt_diagnostics.md) and
[verified Windows upload](2026-10-27_mqtt_diagnostic_upload.md) remain the
applicable source/build records. No source, model, credentials, temporal guard
or scoring changes follow from this positive startup excerpt. Full valid
six-trial PyTorch movement evaluation remains pending.

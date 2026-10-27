# Post-upload sensor, Wi-Fi, TLS and MQTT runtime

Reviewed October 27, 2026 (Asia/Manila). The user supplied serial output after
the [successful ST-LINK programming/verification](2026-10-27_coremqtt_stlink_upload.md).
This report transcribes selected observations and readings; it is not an
independent physical capture or a byte-exact archived transcript. The
[structured evidence](2026-10-27_coremqtt_runtime.json) retains seven printed
readings immediately preceding acknowledgements 1–7.

## Observed positive path

| Stage | Supplied output |
| --- | --- |
| RW007 Wi-Fi | WIFI CONNECTED; LINK UP; DHCP START result=0 |
| DHCP | BOUND; IP 192.168.1.17; netmask 255.255.255.0; gateway 192.168.1.1 |
| ICM20608 | WHO_AM_I 0xAF; INITIALIZED SUCCESSFULLY; six numeric sensor fields |
| Network/time | DNS/TCP connected; UTC ready |
| TLS | TLS verified: TLSv1.2 |
| MQTT | CONNACK accepted; topic activity/sensor; QoS1 |
| Publication acknowledgement | Seven consecutive PUBACK IDs and delivery counts, both 1–7 |

The decisive MQTT excerpt, with intervening sensor/Ethernet diagnostics omitted,
is transcribed as:

```text
[RTSpark] TLS verified: TLSv1.2
[RTSpark] MQTT CONNACK accepted; topic activity/sensor QoS1
[RTSpark] MQTT PUBACK id=1 count=1
[RTSpark] MQTT PUBACK id=2 count=2
[RTSpark] MQTT PUBACK id=3 count=3
[RTSpark] MQTT PUBACK id=4 count=4
[RTSpark] MQTT PUBACK id=5 count=5
[RTSpark] MQTT PUBACK id=6 count=6
[RTSpark] MQTT PUBACK id=7 count=7
```

All six fields of each selected reading are finite. In the reviewed source,
`rtspark_mqtt.c` logs TLS verification only after a successful completed
handshake and a zero certificate-verification result. It logs each PUBACK only
after `rtspark_packet_puback` validates the acknowledgement against the pending
packet ID. These source checks explain the log's positive-path meaning; they do
not add observations missing from the supplied capture.

The source/build/upload chain links this observation to the coreMQTT 2.3.1
integration. The excerpt begins during JOIN and omits the startup version banner;
that banner is not claimed as independently observed here. No reflash or new
firmware change is needed to proceed with downstream checks based on this
successful reported programming and runtime sequence.

## Scope and next checks

This establishes the reported sensor initialization, Wi-Fi/DHCP lease, verified
TLS connection, accepted MQTT login and broker acknowledgements after upload.
PUBACK confirms broker receipt; it does not by itself establish classifier
processing, a database insert or an Android update. The selected JSON readings
are local serial prints, not a separate broker-side payload capture.

There are no per-line timestamps. The seven acknowledgements do not measure
one-second cadence, extended uptime or reconnection behavior. No ground-truth
activity interval is supplied, so this is not a movement-accuracy evaluation.
The original Random Forest movement score remains historical evidence.

Keep the board powered. In a separate PowerShell window, connect to the existing
Ubuntu host, then run the read-only local subscriber and latest-API checks:

```bash
mosquitto_sub -h 127.0.0.1 -p 1883 -t activity/sensor -C 5 -W 15 -v
curl -sS --max-time 10 http://127.0.0.1:5000/api/activity/latest
```

Inspect the actual returned sensor messages and a fresh prediction ID/UTC time.
Then compare the phone's Live/History view and perform a new independently timed
PyTorch walking/jogging session. These subsequent results remain pending.

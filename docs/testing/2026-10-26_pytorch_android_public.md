# PyTorch public-service and Android display checks

Reviewed October 26, 2026 from the user's Windows PowerShell transcript and two
attached Android screenshots. The screenshots are preserved byte-for-byte
under `images/`; original filenames and SHA-256 hashes are in the
[structured evidence](2026-10-26_pytorch_android_public.json). The reviewer did
not independently connect to the server or operate the phone.

## Public checks after the model cutover

The user pulled integration commit `c7488f8` and ran
`python tools/validation/check_cloud_services.py`. The supplied checker output
is timestamped **2026-10-26 04:18:02.877096 UTC**.

| Check | Supplied result |
| --- | --- |
| MQTT port 8883 TLS | PASS; TLSv1.2, ECDHE-RSA-AES256-GCM-SHA384, lab CA and hostname verification enabled |
| HTTPS latest | PASS; HTTP 200, schema valid, ID 6060 created 04:17:56 UTC |
| HTTPS history, limit 1 | PASS; HTTP 200, one valid record, also ID 6060 at 04:17:56 UTC |

The record is 6.877096 seconds older than the checker's final output time; that
is record age at report completion, not measured request latency. ID 6060 is
later than deployment-observed ID 5942 and the subsequent local API ID 5943.

`firmware_database_updates_verified: false` is intentional in this read-only
checker. It performs a TLS handshake on the MQTT port but sends **zero MQTT
application bytes**, including no CONNECT or PUBLISH. Its HTTPS operations are
GETs. This flag is not a failing result, and the checker alone does not verify
board-originated database writes or the firmware running on the board. That
scope remains separate from the earlier actual sensor/deployment evidence.

## Android Live and History

The supplied Live screen shows:

- **LIVE**, Walking, confidence **99.9%**.
- Prediction **6067**, recorded October 26, 2026 at **12:18:36 PM local time**.
- UI text: Updated 5s ago; checking every 5 seconds. This is displayed status,
  not an independently instrumented polling-rate measurement.

![Android Live after PyTorch cutover](images/2026-10-26_pytorch_android_live.jpeg)

The History header reports **50 recent, 50 walking, 0 running**. Five complete
cards are visible, all Walking at displayed confidence 99.9%:

| ID | Displayed local time | UTC if Asia/Manila |
| ---: | --- | --- |
| 6068 | 12:18:41 PM | 04:18:41 |
| 6067 | 12:18:36 PM | 04:18:36 |
| 6066 | 12:18:31 PM | 04:18:31 |
| 6065 | 12:18:26 PM | 04:18:26 |
| 6064 | 12:18:21 PM | 04:18:21 |

![Android History after PyTorch cutover](images/2026-10-26_pytorch_android_history.jpeg)

Record 6067 appears consistently in both screens. The five visible History
cards are newest first, with five-second recorded-time gaps. That demonstrates
displayed history ordering, not the exact sensor acquisition interval or UI
interaction latency. The 50-record total is the app's summary; all 50 cards
were not individually inspected in this screenshot.

The screenshots' IDs/times are later than checker ID 6060, so they are not an
exact server-row match to that earlier check. The checker also omits labels
and confidence from its printed summary. No exact API-row comparison for 6067
or 6068 is claimed here. These images show the Android UI following the prior
physical-phone check; the Xiaomi target was established in earlier evidence,
not newly identified by these standalone screen captures.

## Model attribution and remaining work

These observations follow the [successful PyTorch deployment](2026-10-26_pytorch_cloud_deployment.md)
with model SHA-256
`be60d75d699a6bc317806426c2ec7e47432c2e907e0ceafbab145ba16cce3331`.
Individual database rows have no model-version column, so attribution uses the
recorded deployment and timeline. The earlier 98.1% timed result remains a
Random Forest result. Confidence 99.9% and 50 predicted Walking records are
not measured movement accuracy; actual activity was not labelled here.

Public TLS/HTTPS access and post-cutover Android Live/History display are now
observed. No new coreMQTT firmware flashing/runtime, actual CubeMX regeneration
or labelled live PyTorch movement trial is shown. The handout's conflicting
RT-Spark versus LSM6DSL/ISM43362/SPI3/AC6 reference instructions also remain
unresolved; see [HANDOUT_ALIGNMENT.md](../HANDOUT_ALIGNMENT.md).

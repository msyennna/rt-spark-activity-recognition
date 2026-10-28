# Post-upload Ubuntu broker and prediction API check

Reviewed October 27, 2026 (Asia/Manila). Following successful ST-LINK
programming and the [sensor/TLS/PUBACK runtime capture](2026-10-27_coremqtt_runtime.md),
the user supplied output from a subscriber and local API request on Ubuntu.
This report transcribes those observations; the reviewer did not issue the
commands on the server. The [JSON evidence](2026-10-27_coremqtt_cloud_check.json)
retains all five sensor objects and the complete returned prediction object.

## Observations

| Check | Supplied result |
| --- | --- |
| Local Mosquitto subscriber | Five messages on activity/sensor, fulfilling `-C 5`; command returned to the Ubuntu prompt |
| Sensor payload | All five objects contain the six expected finite numeric acceleration/gyro fields |
| Latest local API record | ID 6746, WALKING, activity_id 0 |
| Confidence | 0.9989190101623535, displayed to one decimal percent as 99.9% |
| Database/API timestamp | October 27, 2026, 01:32:41 UTC |
| Philippine-time equivalent | October 27, 2026, 09:32:41 AM Asia/Manila |

The returned API body is transcribed as:

```json
{"activity":"WALKING","activity_id":0,"confidence":0.9989190101623535,"created_at":"2026-10-27 01:32:41","id":6746}
```

This is later than the previously preserved public-check record 6060 at
October 26, 04:17:56 UTC. In the reviewed application, `/api/activity/latest`
reads the newest SQLite `activity_records` row. Thus the supplied output
supports current post-upload broker reception and a newer database-backed API
prediction in the user's execution sequence. It is not an independent direct
SQLite inspection or an exact association between a specific five-message
subscriber capture and the returned model window.

The `curl` request used **loopback HTTP**, not public HTTPS. No HTTP status
header or independent request/check timestamp was included, so no new public
certificate test or precisely measured response age is claimed. The supplied
record time is preserved rather than replaced with the review clock. There
are no per-message timestamps to measure the one-second publish interval.

## Interpretation and remaining work

Together with the preceding programmed-board and MQTT acknowledgement evidence,
this closes the immediate post-upload Ubuntu broker/local-API readback step.
No cloud deployment or source change was needed to obtain the result. The
previous verified PyTorch deployment remains the model-attribution basis;
this API response does not independently print its checkpoint hash.

The WALKING value is a model prediction. No actual activity label was supplied
for this observation; **99.9% confidence is not a movement-accuracy score**.
The new labelled PyTorch walking/jogging trial remains pending. Earlier Random
Forest live-test results retain their original attribution.

Next keep the board powered and open the existing app on the physical Android
phone. Capture Live with a current prediction ID at least 6746 (normally later,
as new records arrive) and the newest-first History list. The display should
advance while the board continues publishing; it need not remain on ID 6746.
A later screenshot is not an exact independent match to this API object unless
the corresponding row is also captured. The app formats UTC using the device's
current timezone and the confidence to one decimal percent.

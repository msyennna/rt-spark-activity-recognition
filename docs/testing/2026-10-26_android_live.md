# Android Live activity and API verification: October 26, 2026

## Evidence and method

The user supplied two images and local API output after being asked to open
**Live activity**, take a screenshot, wait about 15 seconds without tapping
Refresh, take another screenshot, and then read recent history in Ubuntu.
This report was reviewed on October 26 in Asia/Manila. The prediction timestamps
in the supplied API are October 25 UTC.

Images reviewed:

- `image(20261025-164241).png`
- `image(20261025-164254).png`

The screenshots remain in the user's attachments. This repository retains the
visible observations as text and the supplied API JSON; it does not publish
the images. In a subsequent reply, the user explicitly identified the target
as the virtual phone inside Android Studio. These observations are therefore
classified as emulator evidence.

The user supplied the output of this read-only Ubuntu command:

```bash
curl -sS --max-time 10 "http://127.0.0.1:5000/api/activity/history?limit=10"
```

The response is retained in
[`2026-10-26_android_live_api.json`](2026-10-26_android_live_api.json).

## Visible Live display

| Image | Status | Prediction ID | Activity | Confidence | Displayed recorded time |
| --- | --- | ---: | --- | --- | --- |
| 164241 | LIVE | 4874 | Walking | 67.5% | Oct 25, 2026 · 4:42:26 PM |
| 164254 | LIVE | 4877 | Walking | 67.5% | Oct 25, 2026 · 4:42:41 PM |

Both images show **Live activity**, the LIVE badge, and
**Updated 2s ago · checking every 5 seconds**. Both show the Refresh button in
its normal state and the note that the app refreshes automatically while open.
The displayed prediction ID advances by three and its recorded timestamp
advances by 15 seconds.

This supplies the requested Live-update observation and is consistent with
automatic refreshing under the requested hands-off procedure. The images do
not record intervening user input or independently measure wall-clock elapsed
time, polling precision, or end-to-end latency. Their filenames are labels,
not an independently verified timing source.

## Direct match to the local server API

The supplied response has ten unique IDs, 4884 through 4875 newest first.
All records are WALKING (activity_id 0), confidence 0.675. The UTC completion
timestamps descend in five-second steps from `2026-10-25 16:43:16` to
`2026-10-25 16:42:31`.

**Live prediction 4877 matches the local API exactly:**

| Field | Second Live screenshot | Supplied API |
| --- | --- | --- |
| Prediction ID | 4877 | 4877 |
| Activity | Walking | WALKING / activity_id 0 |
| Confidence | 67.5% | 0.675 |
| Recorded time | Oct 25, 2026 · 4:42:41 PM | 2026-10-25 16:42:41 UTC |

The displayed clock fields correspond to UTC for this record. The actual
device timezone setting was not independently inspected. The app source
converts UTC into the device's default timezone.

The first screenshot's record 4874 falls immediately before the ten returned
API rows; that first record was not independently matched against the supplied
response. The API includes seven records newer than 4877, with completion
timestamps extending another 35 seconds. This is consistent with later
prediction inserts; it does not measure when either screenshot was captured.

The count, unique IDs, descending ID order, finite confidence values, class
mapping, timestamp format, five-second API gaps, screenshot timestamp
progression and record-4877 match were checked before publishing this report.
These are checks on user-supplied evidence, not a new live server query by the
reviewer.

## Result and remaining scope

- Current Live status is visible in both images.
- The displayed prediction advances between observations.
- Live record 4877 matches the supplied local API by ID, activity, confidence
  and timestamp.
- The earlier History/local API match for record 4754 remains documented in
  [the History report](2026-10-26_android_history.md).
- The supplied sequence supports the requested automatic-refresh check;
  precise refresh timing and absence of input between still images were not
  independently instrumented.
- Device type is confirmed by the user's follow-up: **Android Studio emulator**
  ("virtual phone inside android studio"). This completes device attribution for
  the supplied Live screenshots. A later [physical-phone History photo and
  device-selector image](2026-10-26_android_phone.md) show History on physical
  hardware and Xiaomi 25028RN03A selected in Android Studio. A subsequent phone
  Live image shows LIVE and record 5167, matching local API WALKING, 67.5%, and
  02:08:39 UTC = 10:08:39 AM Asia/Manila. The physical-phone comparison is
  complete; these emulator Live images retain their original attribution.
- Earlier power-cycle/stale-data recovery remains user-confirmed. These two
  Live images do not repeat a power interruption.
- Local HTTP curl does not independently repeat public HTTPS certificate
  verification. The reviewed app source uses the configured HTTPS endpoint.
- A later [firmware PUBACK transcript](2026-10-26_cubemx_puback.md) supplies
  acknowledgement evidence independently of these images. Actual CubeMX
  regeneration remains a separate integration check.

The 67.5% value is per-prediction confidence. It is separate from the audited
movement trial's 51/52 agreement (98.1%). The model has no resting class, and
these images do not establish the participant's actual activity.

This milestone changes evidence and documentation only. Android, firmware,
model and deployed services are unchanged.

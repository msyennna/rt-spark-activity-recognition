# Android History display observation: October 26, 2026

## Evidence and scope

The user supplied `image(20261025-161831).png` while checking the Android app
after the audited walking/jogging trial. This observation is recorded on
October 26 in Asia/Manila. It describes the visible image, not an independently
operated device or an API read performed by the reviewer.

The image shows the RTSpark Activity Monitor **Activity history** page and a
Refresh button. The history counter reads **50 recent · 50 walking · 0 running**.
Six cards are visible; the full set of 50 has not been independently recounted
from this image.

## Visible cards

Times below are transcribed exactly as displayed. The device timezone was not
supplied, so these values are not labelled UTC or Asia/Manila.

| Prediction ID | Displayed timestamp | Activity | Confidence |
| --- | --- | --- | --- |
| 4585 | Oct 25, 2026 · 4:18:15 PM | Walking | 67.5% |
| 4584 | Oct 25, 2026 · 4:18:10 PM | Walking | 67.5% |
| 4583 | Oct 25, 2026 · 4:18:05 PM | Walking | 67.5% |
| 4582 | Oct 25, 2026 · 4:18:00 PM | Walking | 67.5% |
| 4581 | Oct 25, 2026 · 4:17:55 PM | Walking | 67.5% |
| 4580 | Oct 25, 2026 · 4:17:50 PM | Walking | 67.5% |

The six visible IDs and timestamps descend in order. Adjacent displayed
timestamps are five seconds apart. This confirms populated, newest-first
visible history; it does not measure the app's automatic refresh interval.

The reviewed `ActivityRecord.kt` converts API UTC timestamps into the device's
`ZoneId.systemDefault()` for display. A timezone difference can therefore
change the displayed date/time without changing prediction IDs. The image
alone does not establish the device timezone or current data age.

## Relationship to the completed trial

The audited RUNNING trial windows were IDs 4101–4109, 4131–4138 and 4160–4168.
The visible History IDs are much later, at 4580–4585. With the app's latest-50
view and continued prediction inserts, older trial records are expected to
leave that view. The visible zero-Running counter does not revise or replace
the preserved trial evidence.

The current 67.5% values are per-prediction confidence. The trial's 98.1%
was independently recomputed agreement on 51/52 eligible windows. These are
different quantities. A resting board can still receive a Walking or Running
label because the model has no resting class; this screenshot does not label
the participant's actual activity at capture.

See the [audited trial report](2026-10-25_cubemx_walk_jog_trial.md) for the
unchanged original data, window selection and single scored error.

## Follow-up: matching History to the local server API

The user next supplied `image(20261025-163234).png` and the output of this
read-only command run in Ubuntu:

```bash
curl -sS --max-time 10 "http://127.0.0.1:5000/api/activity/history?limit=10"
```

The second image is still **Activity history**. It shows a **Refreshing…**
button and the same 50 recent / 50 Walking / 0 Running counter. Its visible
cards are:

| Prediction ID | Displayed timestamp | Activity | Confidence |
| --- | --- | --- | --- |
| 4754 | Oct 25, 2026 · 4:32:23 PM | Walking | 67.5% |
| 4753 | Oct 25, 2026 · 4:32:18 PM | Walking | 67.5% |
| 4752 | Oct 25, 2026 · 4:32:13 PM | Walking | 67.5% |
| 4751 | Oct 25, 2026 · 4:32:08 PM | Walking | 67.5% |
| 4750 | Oct 25, 2026 · 4:32:03 PM | Walking | 67.5% |
| 4749 | Oct 25, 2026 · 4:31:58 PM | Walking | 67.5% |

The supplied API response is retained in
[`2026-10-26_android_history_api.json`](2026-10-26_android_history_api.json).
It contains ten distinct IDs, 4763 through 4754 newest first, all WALKING
(activity_id 0) with confidence 0.675. Adjacent UTC completion timestamps are
five seconds apart, from 16:32:23 through 16:33:08 on October 25.

**Prediction 4754 matches the visible History card:** ID 4754, WALKING,
0.675 = 67.5%, and API time `2026-10-25 16:32:23` UTC corresponding to the
displayed `Oct 25, 2026 · 4:32:23 PM`. The matching clock fields are consistent
with a UTC display for this record; the device timezone setting was not
independently inspected. The other five visible cards are older than the
ten-row API response and were not independently matched in this follow-up.

The API includes nine records newer than the top visible card. Their completion
timestamps span another 45 seconds. This is consistent with continuing
prediction inserts between the displayed record and the later API output.
It does not establish the screenshot's exact capture time, app latency, or
automatic refresh behavior.

Compared with the earlier image (top ID 4585), the follow-up displays a newer
set of predictions (top ID 4754). These two images establish that the displayed
History content changed between observations. They do not distinguish manual
from automatic refreshing. **Refreshing…** alone does not prove a completed
request or show whether it was triggered manually.

This is a History-to-local-API match using user-supplied evidence. The reviewer
did not operate the device or query the deployed server. The local curl command
does not independently repeat the public HTTPS, CA or hostname-verification
check. The image does not show the **Live activity** page or LIVE status.

## Subsequent Live check and remaining evidence

The later [Live/API report](2026-10-26_android_live.md) records two Live images
with LIVE status, IDs advancing from 4874 to 4877, and an exact local API match
for record 4877. This completes the requested Live display/record comparison
and supports the requested automatic-refresh observation. Precise polling and
intervening input were not independently instrumented by the still images.

The user subsequently confirmed that this workflow uses the virtual phone
inside Android Studio. These app observations are classified as emulator
evidence. A subsequent [physical-phone History photograph](2026-10-26_android_phone.md)
shows new IDs 5027–5023, with Xiaomi 25028RN03A selected in Android Studio.
The phone's Live status and current API record match remain pending.

Earlier power-cycle/stale-data recovery was confirmed by the user on October 23.
This single History image is not a repeat of that recovery check. Firmware
PUBACK capture and CubeMX regeneration remain separate checks in
[STM32_INTEGRATION.md](../STM32_INTEGRATION.md).

This milestone updates documentation only. It changes no Android, firmware,
model or service source.

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

## Remaining Android evidence

1. Show the **Live activity** page with the LIVE badge and current prediction ID.
2. Compare that ID, activity and confidence with recent server API records.
3. Observe advancing IDs/timestamps over time to confirm automatic refresh.
4. Record the actual device type. Earlier context involved Android Studio and
   an emulator; a cropped app image alone does not establish a physical-phone run.

Earlier power-cycle/stale-data recovery was confirmed by the user on October 23.
This single History image is not a repeat of that recovery check. Firmware
PUBACK capture and CubeMX regeneration remain separate checks in
[STM32_INTEGRATION.md](../STM32_INTEGRATION.md).

This milestone updates documentation only. It changes no Android, firmware,
model or service source.

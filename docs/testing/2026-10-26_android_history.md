# Android History and physical-phone verification: October 26, 2026

## Evidence and scope

The supplied `image(20261025-161831).png` records the Android History display
after the earlier Random Forest walking/jogging trial. This observation is
recorded on October 26 in Asia/Manila. It describes the visible image; it does
not document an independent device operation or API query.

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

The follow-up evidence consists of `image(20261025-163234).png` and the output
of this read-only command run in Ubuntu:

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

This History-to-local-API match is based on supplied screenshots and terminal
output, without an independent device operation or deployed-server query.
The local curl command
does not independently repeat the public HTTPS, CA or hostname-verification
check. The image does not show the **Live activity** page or LIVE status.

## Subsequent Live check and remaining evidence

Subsequent emulator observations recorded two Live images with LIVE status and
IDs advancing from 4874 to 4877. The
[retained API extract](2026-10-26_android_live_api.json) contains the matching
server record 4877. The reported sequence supports automatic refresh; the
still images did not independently measure polling precision or intervening
input, and the API extract alone does not establish the displayed UI state.

This workflow used the virtual phone inside Android Studio. These app
observations retain their emulator attribution. A subsequent
[physical-phone History observation](#physical-phone-verification)
shows new IDs 5027–5023, with Xiaomi 25028RN03A selected in Android Studio.
A later phone Live image shows LIVE and record 5167, matching supplied local
API WALKING, 67.5%, and 02:08:39 UTC = 10:08:39 AM Asia/Manila. The
physical-phone display/record comparison is complete; these earlier History
images retain their emulator attribution.

Earlier power-cycle/stale-data recovery was reported on October 23.
This single History image is not a repeat of that recovery check. Firmware
[PUBACK capture](2026-10-26_cubemx_puback.md) has since been supplied and checked;
it is separate from the images above. CubeMX regeneration remains a separate
check in the [firmware verification guide](../../firmware/README.md#integration-verification).

The physical-phone observations and their evidence limits are recorded below.

## Physical-phone verification

### Supplied evidence

Physical-phone evidence recorded after the Android Studio emulator checks
consists of:

- `4fd038b6-a35b-4e46-81d2-1f0ee4306999.jpeg`: a photograph of a physical phone
  displaying RTSpark Activity Monitor's **Activity history** page.
- `image(20261026-015811).png`: Android Studio's device selector showing
  **Xiaomi 25028RN03A**, with a green device indicator.

The photograph establishes that History is displayed on physical phone hardware.
The separate selector supplies the selected device's model identifier. The
phone's own About screen was not supplied, so the model is recorded exactly
as shown in Android Studio; no retail model name is inferred.

The original photographs are not included in this repository. The observations
below are transcriptions of the supplied images.

### Visible History records

The header reads **50 recent · 50 walking · 0 running**. Five complete cards are
readable. The counter's full set of 50 has not been independently recounted.

| Prediction ID | Displayed timestamp | Activity | Confidence |
| --- | --- | --- | --- |
| 5027 | Oct 26, 2026 · 9:56:56 AM | Walking | 67.5% |
| 5026 | Oct 26, 2026 · 9:56:51 AM | Walking | 67.5% |
| 5025 | Oct 26, 2026 · 9:56:46 AM | Walking | 67.5% |
| 5024 | Oct 26, 2026 · 9:56:40 AM | Walking | 67.5% |
| 5023 | Oct 26, 2026 · 9:56:35 AM | Walking | 67.5% |

Visible IDs and times descend newest first. Adjacent displayed completion-time
gaps are 5, 5, 6 and 5 seconds. This is a display observation, not a sensor-rate
or app-polling measurement. The times are transcribed as displayed; the device's
timezone setting and corresponding API UTC values were not supplied here.

The **History** navigation button is selected. **Live activity** is available
at the bottom left. This photograph does not show a Live prediction card or
LIVE badge.

### Follow-up: Live prediction matches the server

The follow-up evidence includes `9d122072-3f59-4139-90ba-d44ef0ff54f9.jpeg` and output
from this read-only Ubuntu command:

```bash
curl -sS --max-time 10 "http://127.0.0.1:5000/api/activity/history?limit=20"
```

The phone image shows **Live activity**, a **LIVE** badge, **Walking**,
**Confidence 67.5%**, **Prediction #5167**, and recorded local time
**Oct 26, 2026 · 10:08:39 AM**. Its status text reads
**Updated 6s ago · checking every 5 seconds**.

The supplied response is preserved in
[`2026-10-26_android_phone_api.json`](2026-10-26_android_phone_api.json).

| Field | Physical-phone Live display | Supplied local API |
| --- | --- | --- |
| Prediction ID | 5167 | 5167 |
| Activity | Walking | WALKING / activity_id 0 |
| Confidence | 67.5% | 0.675 |
| Recorded time | Oct 26, 2026 · 10:08:39 AM | 2026-10-26 02:08:39 UTC |

**The physical-phone Live record matches the server by ID, activity, confidence
and timestamp after timezone conversion.** October 26 at 02:08:39 UTC is
10:08:39 AM in Asia/Manila (UTC+08:00). The displayed clock is consistent with
this conversion; the actual timezone setting was not independently inspected.

The response contains 20 distinct IDs, 5169 through 5150 newest first, all
WALKING with confidence 0.675. Its completion timestamps run from 02:07:13 to
02:08:49 UTC. The 19 adjacent gaps are eighteen 5-second intervals and one
6-second interval (5151 to 5152). Count, uniqueness, descending IDs, class,
confidence, timestamp parsing, gaps and the record-5167 Manila conversion were
checked before publication.

Two API rows are newer than the displayed record, ending at 02:08:49 UTC.
That is consistent with continuing inserts after record 5167; it does not
measure the screenshot's capture time or end-to-end latency.

### Result and remaining scope

**Physical-phone installation, populated History, LIVE display, and the
phone-to-local-API record match are observed.** The matching fields establish the
physical-phone display/record comparison for this observation.

The prior History photo had no accompanying API response, so IDs 5023–5027
remain visual observations rather than independently matched rows. The new
Live/API match is specifically record 5167. A single Live image does not
independently demonstrate hands-off refresh or a power-interruption recovery
on this phone; the earlier emulator refresh sequence and reported
recovery retain their original scope.

This report is based on supplied images and terminal output; it does not
document a separate phone operation or independent query of the deployed API.
Local HTTP curl does not independently repeat
public HTTPS certificate verification; the reviewed app source uses the
configured HTTPS endpoint.

Earlier emulator observations reported Live record 4877; the
[retained API extract](2026-10-26_android_live_api.json) contains the matching
server row. The [History/API check](2026-10-26_android_history.md) records the
earlier match for 4754. These remain emulator observations. Earlier
power-cycle recovery was reported separately from the new physical-phone
Live/API match.

The 67.5% values are prediction confidence, separate from the earlier Random
Forest movement trial's 51/52 agreement (98.1%). The model has WALKING/RUNNING labels
and no resting class. The participant's actual activity is not established
by the photograph.

The [firmware PUBACK transcript](2026-10-26_cubemx_puback.md) records broker
acknowledgements separately from the phone images. Actual CubeMX generation is
covered by the separate [generation review](2026-10-27_cubemx_generation_review.md).

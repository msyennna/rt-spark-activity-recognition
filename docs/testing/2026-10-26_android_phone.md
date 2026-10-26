# Physical Android phone: History and Live/API verification, October 26, 2026

## Supplied evidence

The user supplied these images after completing the Android Studio emulator
checks and being asked to run the same app on a physical Android phone:

- `4fd038b6-a35b-4e46-81d2-1f0ee4306999.jpeg`: a photograph of a physical phone
  displaying RTSpark Activity Monitor's **Activity history** page.
- `image(20261026-015811).png`: Android Studio's device selector showing
  **Xiaomi 25028RN03A**, with a green device indicator.

The photograph establishes that History is displayed on physical phone hardware.
The separate selector supplies the selected device's model identifier. The
phone's own About screen was not supplied, so the model is recorded exactly
as shown in Android Studio; no retail model name is inferred.

The original images remain in the user's attachments. This repository records
visible observations as text and does not publish or modify the photographs.

## Visible History records

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

## Follow-up: Live prediction matches the server

The user then supplied `9d122072-3f59-4139-90ba-d44ef0ff54f9.jpeg` and output
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

## Result and remaining scope

**Physical-phone installation, populated History, LIVE display, and the
phone-to-local-API record match are observed.** This completes the requested
physical-phone display/record comparison.

The prior History photo had no accompanying API response, so IDs 5023–5027
remain visual observations rather than independently matched rows. The new
Live/API match is specifically record 5167. A single Live image does not
independently demonstrate hands-off refresh or a power-interruption recovery
on this phone; the earlier emulator refresh sequence and user-confirmed
recovery retain their original scope.

The reviewer inspected user-supplied evidence and did not operate the phone
or query the deployed API. Local HTTP curl does not independently repeat
public HTTPS certificate verification; the reviewed app source uses the
configured HTTPS endpoint.

The previous emulator [Live/API check](2026-10-26_android_live.md) remains valid
for record 4877, and its [History/API check](2026-10-26_android_history.md)
remains valid for record 4754. Those emulator observations are not relabelled
as physical-phone evidence. Earlier power-cycle recovery remains user-confirmed
and is separate from the new physical-phone Live/API match.

The 67.5% values are prediction confidence, separate from the audited
movement trial's 51/52 agreement (98.1%). The model has WALKING/RUNNING labels
and no resting class. The participant's actual activity is not established
by the photograph.

This milestone changes documentation only. A later
[firmware PUBACK transcript](2026-10-26_cubemx_puback.md) supplies accepted
acknowledgement evidence independently of the phone images. Actual CubeMX
regeneration remains a separate check in
[STM32_INTEGRATION.md](../STM32_INTEGRATION.md).

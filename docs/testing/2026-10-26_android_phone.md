# Physical Android phone: History display, October 26, 2026

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

## Result and remaining check

**Physical-phone installation and populated History display are observed.**
This advances the device check beyond the previously confirmed emulator.

A current phone **Live activity** image and matching recent API record are
still needed to verify that phone's LIVE status and displayed latest record.
No API response accompanied these two images, so none of IDs 5023–5027 is
claimed as an independent phone-to-API match.

The previous emulator [Live/API check](2026-10-26_android_live.md) remains valid
for record 4877, and its [History/API check](2026-10-26_android_history.md)
remains valid for record 4754. Those emulator observations are not relabelled
as physical-phone evidence. Earlier power-cycle recovery remains user-confirmed;
the new photograph does not repeat a power interruption or demonstrate
automatic refresh over time.

The 67.5% values are prediction confidence, separate from the audited
movement trial's 51/52 agreement (98.1%). The model has WALKING/RUNNING labels
and no resting class. The participant's actual activity is not established
by the photograph.

This milestone changes documentation only. Firmware PUBACK capture and actual
CubeMX regeneration remain separate checks in
[STM32_INTEGRATION.md](../STM32_INTEGRATION.md).

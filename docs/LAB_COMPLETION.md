# Laboratory Activity 3: current completion review

Reviewed October 26, 2026 (Asia/Manila), against all seven pages of the supplied
`BCA180 ESP - Laboratory Activity 3 (3).docx`. Its SHA-256 is
`8efc5cf8d9c75abff183d1f76581beaded74cc0c1460bce08006ef6a08ef6bc8`.
This review distinguishes source/build results from deployment and physical
evidence. The earlier working system does not establish that new code is live.

## Current alignment

| Handout area | Current evidence | Remaining action |
| --- | --- | --- |
| Cloud VM, broker, database and API | AWS EC2, Mosquitto, SQLite and Flask are listed choices. Earlier MQTT reception, saved rows and API responses establish the deployed baseline. | Deploy the new classifier and verify fresh rows and API output. |
| TensorFlow or PyTorch; all axes, temporal learning, lightweight design | Random Forest has been replaced in the repository by a trained 450-parameter PyTorch GRU over ordered five-reading windows and all six axes. Verified Kaggle dataset provenance and timestamp-based preparation are recorded. | The Ubuntu service still has the earlier model until cutover. Use the deployment runbook. |
| Rigorous model testing | New PyTorch development OOF: 2219/2258 (98.27%). Untouched latest-two-day holdout: 1197/1300 (92.08%). Ten actual model/database/API tests passed. | Repeat a labelled live movement test after deployment. This dataset contains one participant; no population-accuracy claim is justified. |
| Sensor and Wi-Fi firmware | The demonstrated RT-Spark has ICM20608 and RW007 on I2C2/SPI2. Earlier physical logs show sensor identity, networking, TLS, accepted MQTT login and PUBACKs. | Resolve the handout's conflicting chip/bus reference instructions. Actual CubeMX regeneration is also pending. |
| AWS coreMQTT and default-config define | Actual unmodified coreMQTT 2.3.1 low-level API now replaces the custom packet encoder/parser. `MQTT_DO_NOT_USE_CUSTOM_CONFIG` is defined. ARM build, five native suites and source guard passed. | Build with the local private configuration and flash the reviewed firmware, then collect new physical logs. The earlier custom-encoder PUBACKs are historical. |
| One reading/message per second | Firmware schedules a nominal one-second publish interval. Earlier excerpt contains 17 accepted PUBACKs, IDs 1482–1498. | Obtain a timestamped capture to measure cadence. An untimed log cannot establish it. |
| Android smartphone, live predictions and history | Kotlin, Android Studio and Volley match listed choices. Physical Xiaomi History was observed; phone Live record 5167 matches the supplied API by label, confidence and time conversion. | Verify fresh PyTorch predictions on the phone after cutover. Network work remains asynchronous; no UI rewrite is required by the handout. |

## Handout conflict requiring correction

Page 1 names RT-Spark and page 2 names VSCode PlatformIO. Later reference steps
name LSM6DSL, ISM43362, SPI3, CMSIS driver packs, an AC6 `solution.yml` and a
`WiFi_ISM43362_HW.c` reference file that was not supplied. These instructions
do not consistently describe the demonstrated board/project.

Changing a driver name or enabling SPI3 cannot replace the installed
ICM20608/RW007 hardware. Ask the instructor for the corrected instructions for
RT-Spark, or confirmation that a different physical board is intended. This is
an unresolved literal-alignment issue, not an assumed exception. The actual
coreMQTT source integration does not itself run the pack/component-editor steps.
See [HANDOUT_ALIGNMENT.md](HANDOUT_ALIGNMENT.md).

## Evidence and model attribution

- [New PyTorch preparation, training, splits and checkpoint](../cloud/classifier/README.md)
- [Machine-readable PyTorch training report](testing/pytorch_training_2026-10-26.json)
- [New coreMQTT source/build checks and Linux TLS-test skips](testing/2026-10-26_coremqtt_port.md)
- [Cloud cutover and physical validation instructions](DEPLOY_HANDOUT_UPDATES.md)
- [Earlier physical PUBACK report and full transcript](testing/2026-10-26_cubemx_puback.md)
- [Earlier physical-phone History and Live/API report](testing/2026-10-26_android_phone.md)
- [Earlier independently timed trial and raw-data audit](testing/2026-10-25_cubemx_walk_jog_trial.md)
- [Detailed integration history](STM32_INTEGRATION.md)

The earlier 51/52 result (98.1%) came from six valid timed trials using
**Random Forest**, with one participant holding the board in the right hand.
It is retained unchanged and must not be attributed to the new PyTorch model.
The PyTorch final deployment checkpoint was refitted on all available days
only after holdout evaluation; the held-out score belongs to the earlier
holdout-trained checkpoint, not a re-evaluation of the final refit.

Both models have WALKING/RUNNING classes only. Stationary input can still be
classified as one of them. Confidence is not measured accuracy. The handout
does not separately require a resting class or specify a minimum score.

## Completion boundary

Source changes are prepared and tested. No cloud cutover, board flashing,
actual CubeMX regeneration or new PyTorch movement/phone validation occurred
in this update. Strict completion remains pending those applicable actions
and the hardware/toolchain clarification.

The integration checklist includes additional engineering checks such as DHCP
renewal, controlled interruptions and memory/polling measurements. Their
pending status is recorded honestly; the handout does not prescribe every one
as a separate grading requirement, nor a particular video/report format.

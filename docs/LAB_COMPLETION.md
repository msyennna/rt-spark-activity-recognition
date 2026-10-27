# Laboratory Activity 3: current completion review

Reviewed October 27, 2026 (Asia/Manila), against all seven pages of the supplied
`BCA180 ESP - Laboratory Activity 3 (3).docx`. Its SHA-256 is
`8efc5cf8d9c75abff183d1f76581beaded74cc0c1460bce08006ef6a08ef6bc8`.
This review distinguishes source/build results from deployment and physical
evidence. The earlier working system does not establish that new code is live.

The core implementation and planned live evaluation are **complete**, with
the actual-board adaptation documented below. The original uploaded session
has now been independently audited: all six trials are valid, and **29/53
windows are correct (54.7%)**. Walking is **26/26**, while jogging/Running is
**3/27**. Weak jogging recognition remains a performance limitation; no further
repeat is needed to verify this completed session. See the [original-session
audit](testing/2026-10-27_pytorch_walk_jog_audit.md).

## Current alignment

| Handout area | Current evidence | Remaining action |
| --- | --- | --- |
| Cloud VM, broker, database and API | AWS EC2, Mosquitto, SQLite and Flask are listed choices. Deployment, broker reception, database-backed API readback and public TLS/HTTPS checks were previously recorded. The latest timed-test freshness preflight passed. | Complete for the recorded end-to-end checks. The original movement session is now audited. No new Android refresh measurement or checkpoint hash is inferred from that file. |
| TensorFlow or PyTorch; all axes, temporal learning, lightweight design | A trained 450-parameter PyTorch GRU uses ordered five-reading windows and all six axes. Matching deployed model hash and Ubuntu model/contract tests were previously recorded. | Implementation and live evaluation are complete. The original session audit confirms 54.7% accuracy and only 3/27 jogging windows correct. Report weak generalization; further model improvement is a quality-development task. |
| Rigorous model testing | PyTorch development OOF: 2219/2258 (98.27%). Untouched latest-two-day holdout: 1197/1300 (92.08%). The completed live summary reports 29/53 (54.7%), with all six trials marked valid. | Original-session replay now matches all six trial analyses and aggregate metrics. Keep dataset and live scores distinct; all incorrect eligible predictions remain included. Participant coverage remains limited. |
| Sensor and Wi-Fi firmware | The actual board uses ICM20608/RW007 on I2C2/SPI2. Actual CubeMX output review, Windows build, verified ST-LINK upload and sensor/DHCP/TLS/PUBACK runtime were previously recorded. The user chose this hardware without an instructor reply. | Retain the documented reference-hardware difference. Later diagnostic publishing and receipt checks provide additional observations, without establishing the physical cause of earlier interruptions. |
| AWS coreMQTT and default-config define | Unmodified coreMQTT 2.3.1 low-level APIs and `MQTT_DO_NOT_USE_CUSTOM_CONFIG` are integrated. ARM build, five native suites, CubeMX source guard and physical upload/runtime were recorded. Later error diagnostics were built and uploaded, followed by an excerpt containing 620 consecutive PUBACKs. | Preserve source/build/runtime evidence. The acknowledgements alone do not establish classifier accuracy or exact sensor sampling timing. |
| One reading/message per second | Firmware schedules a nominal one-second publish interval. Earlier captures contained delivery gaps. The latest requested power-bank walking receipt check returned 70/70 messages with a maximum interval of 2.016 s and no interval above 2.5 s; six intervals were approximately two seconds. | Report the nominal schedule and observed timing honestly. This short capture does not prove uninterrupted one-second delivery or identify a repair. The classifier's temporal guard remains unchanged. |
| Android smartphone, live predictions and history | Kotlin, Android Studio and Volley match listed choices. Physical-phone Live/History observations and API comparisons were preserved, followed by user confirmation that Android worked after the reconciled firmware upload. | Retain the existing evidence. The new movement summary supplies no new Android screenshot or measured refresh timing; no replacement screenshot is inferred. |

## Hardware/toolchain reference differences

Page 1 names RT-Spark and page 2 names VSCode PlatformIO. Later reference steps
name LSM6DSL, ISM43362, SPI3, CMSIS driver packs, an AC6 `solution.yml` and a
`WiFi_ISM43362_HW.c` reference file that was not supplied. These instructions
do not consistently describe the demonstrated board/project.

Changing a driver name or enabling SPI3 cannot replace the installed
ICM20608/RW007 hardware. On October 26 the user stated that instructor
clarification was not yet available and chose to continue with the actual
board. This supports proceeding with its existing driver/bus configuration,
not claiming instructor approval or literal execution of the different
reference steps. The actual coreMQTT source integration does not itself run
the pack/component-editor steps.
See [HANDOUT_ALIGNMENT.md](HANDOUT_ALIGNMENT.md).

## Evidence and model attribution

- [New PyTorch preparation, training, splits and checkpoint](../cloud/classifier/README.md)
- [Machine-readable PyTorch training report](testing/pytorch_training_2026-10-26.json)
- [New coreMQTT source/build checks and Linux TLS-test skips](testing/2026-10-26_coremqtt_port.md)
- [Cloud cutover and physical validation instructions](DEPLOY_HANDOUT_UPDATES.md)
- [Successful Ubuntu PyTorch deployment and local API result](testing/2026-10-26_pytorch_cloud_deployment.md)
- [Post-PyTorch public checks and Android Live/History screenshots](testing/2026-10-26_pytorch_android_public.md)
- [User's Windows coreMQTT firmware build and CubeMX review preparation](testing/2026-10-26_windows_coremqtt_build.md)
- [Actual CubeMX 6.18.1 generated-file review and source reconciliation](testing/2026-10-27_cubemx_generation_review.md)
- [Successful local Windows rebuild after source reconciliation](testing/2026-10-27_windows_reconciled_build.md)
- [Successful ST-LINK programming and OpenOCD verification](testing/2026-10-27_coremqtt_stlink_upload.md)
- [Post-upload sensor, DHCP, TLS and seven MQTT acknowledgements](testing/2026-10-27_coremqtt_runtime.md)
- [Post-upload Ubuntu broker reception and API record 6746](testing/2026-10-27_coremqtt_cloud_check.md)
- [Post-upload Android operation confirmed by the user](testing/2026-10-27_android_confirmation.md)
- [Stopped PyTorch movement attempt and required stream diagnostics](testing/2026-10-27_pytorch_trial_stopped.md)
- [Stopped-session data audit and later recovered reception/prediction](testing/timed_walk_jog_20261027T020953Z_8619c6/review.md)
- [Trial-time delivery-gap diagnosis and short continuity check](testing/timed_walk_jog_20261027T020953Z_8619c6/delivery_review.md)
- [Independent broker receipt check: interruptions reproduced](testing/2026-10-27_broker_continuity/review.md)
- [Later stationary receipt check: 70 messages at approximately one-second intervals](testing/2026-10-27_stationary_continuity/review.md)
- [Walking receipt comparison: 70 messages with one 3.006-second gap](testing/2026-10-27_walking_continuity/review.md)
- [Diagnostic error reporting, build and test checks](testing/2026-10-27_mqtt_diagnostics.md)
- [Verified Windows diagnostic firmware upload](testing/2026-10-27_mqtt_diagnostic_upload.md)
- [Extended diagnostic run: 620 consecutive acknowledgements](testing/2026-10-27_mqtt_diagnostic_continuation.md)
- [Latest requested power-bank walking receipt check](testing/2026-10-27_power_bank_walk_receipts.md)
- [Completed PyTorch movement session: reported 54.7% result](testing/2026-10-27_pytorch_walk_jog_reported.md)
- [Machine-readable reported summary with completed follow-up audit](testing/2026-10-27_pytorch_walk_jog_reported.json)
- [Independent original-session audit: six valid trials and 54.7% accuracy](testing/2026-10-27_pytorch_walk_jog_audit.md)
- [Machine-readable anonymized original-session audit](testing/2026-10-27_pytorch_walk_jog_audit.json)
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

The three core work areas in the supplied handout are implemented and have
recorded end-to-end evidence: cloud/model, embedded firmware and Android app.
Actual CubeMX generation review, firmware build and verified upload, broker/API
operation and Android Live/History observations are preserved in the linked
records.

The uploaded original movement session now closes the pending audit. All six
trials pass the existing duration, clock-consistency, participant-confirmation,
coverage, uniqueness and cadence rules. Independently selected windows and
aggregate metrics match the saved file exactly: **29/53 correct (54.7%)**.
The earlier stopped attempt remains preserved and is not mixed into this score.

The unresolved performance weakness is jogging/Running recognition at
**3/27 (11.1%)**. It should be discussed honestly in the submitted results.
The handout encourages good generalization but gives no numerical accuracy
pass mark. Completing the implementation and evaluation does not establish
reliable classification across people or environments. Any improvement
should use separate development data and a new evaluation, preserving this
baseline and keeping the older Random Forest score separate.

No further trial or original-session file is missing for this audit.
The supplied handout does not prescribe a report template, video format or
the extra engineering checklist as additional completion gates. Present the
existing code, working demonstration and verified result using the class's
actual submission instructions. Grading and any separate instructor
requirements are not established by this repository checklist.

The documented hardware/toolchain reference differences remain; no instructor
approval or literal use of the different reference hardware is inferred.

The integration checklist includes additional engineering checks such as DHCP
renewal, controlled interruptions and memory/polling measurements. Their
pending status is recorded honestly; the handout does not prescribe every one
as a separate grading requirement, nor a particular video/report format.

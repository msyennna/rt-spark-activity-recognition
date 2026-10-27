# Laboratory Activity 3: current completion review

Reviewed October 27, 2026 (Asia/Manila), against all seven pages of the supplied
`BCA180 ESP - Laboratory Activity 3 (3).docx`. Its SHA-256 is
`8efc5cf8d9c75abff183d1f76581beaded74cc0c1460bce08006ef6a08ef6bc8`.
This review distinguishes source/build results from deployment and physical
evidence. The earlier working system does not establish that new code is live.

The latest participant-reported PyTorch movement session is **COMPLETE**, with
all six trials marked valid and **29/53 correct windows (54.7%)**. Walking was
**26/26**, while jogging/Running was **3/27**; weak jogging recognition is the
main observed limitation. These totals are verified from the pasted summary.
The original `session.json` still needs independent replay before the evidence
audit is complete. See the [reported result](testing/2026-10-27_pytorch_walk_jog_reported.md).

## Current alignment

| Handout area | Current evidence | Remaining action |
| --- | --- | --- |
| Cloud VM, broker, database and API | AWS EC2, Mosquitto, SQLite and Flask are listed choices. Deployment, broker reception, database-backed API readback and public TLS/HTTPS checks were previously recorded. The latest timed-test freshness preflight passed. | Retain those service checks. Audit the original completed movement session; the new console summary does not separately establish Android refresh or a new checkpoint hash. |
| TensorFlow or PyTorch; all axes, temporal learning, lightweight design | A trained 450-parameter PyTorch GRU uses ordered five-reading windows and all six axes. Matching deployed model hash and Ubuntu model/contract tests were previously recorded. | The latest six-trial live summary reports 54.7% accuracy, including only 3/27 jogging windows correct. Audit the original session and document the limitation; no speculative model change is made from the summary. |
| Rigorous model testing | PyTorch development OOF: 2219/2258 (98.27%). Untouched latest-two-day holdout: 1197/1300 (92.08%). The completed live summary reports 29/53 (54.7%), with all six trials marked valid. | Keep dataset scores and live scores distinct. Independently replay the original live session; do not exclude wrong jogging predictions. Both the dataset and live test have limited participant coverage. |
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
- [Machine-readable aggregate summary, original-session replay pending](testing/2026-10-27_pytorch_walk_jog_reported.json)
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

Implementation, CubeMX review, firmware build/verified upload, cloud deployment,
broker/API operation and Android observations are recorded. The earlier
PyTorch movement attempt stopped with invalid prediction coverage; its evidence
remains preserved. A later diagnostic build/upload and short receipt check
provided further connectivity evidence. The latest user-pasted timed-test
summary now reports all six trials complete and valid, with **54.7% window
accuracy** and weak jogging recognition.

The remaining evidence closeout is to obtain and independently replay the
original `session.json`, then include the actual result and limitations in the
laboratory report. The console summary alone is not a raw-row audit, a fresh
model-hash check or an explanation of the classification errors. No confidence
values or raw six-axis motion samples were supplied with it.

The handout does not specify a minimum accuracy score. Protocol completion
should therefore be recorded separately from predictive quality; the low
jogging score must not be hidden or described as reliable recognition. Any
later improvement needs separate development data and a new evaluation,
retaining this completed baseline. The old Random Forest score must not be
substituted for it.

The actual hardware/toolchain reference differences remain documented under
the user's continuation choice. No instructor approval is inferred.

The integration checklist includes additional engineering checks such as DHCP
renewal, controlled interruptions and memory/polling measurements. Their
pending status is recorded honestly; the handout does not prescribe every one
as a separate grading requirement, nor a particular video/report format.

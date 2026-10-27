# Laboratory Activity 3: current completion review

Reviewed October 27, 2026 (Asia/Manila), against all seven pages of the supplied
`BCA180 ESP - Laboratory Activity 3 (3).docx`. Its SHA-256 is
`8efc5cf8d9c75abff183d1f76581beaded74cc0c1460bce08006ef6a08ef6bc8`.
This review distinguishes source/build results from deployment and physical
evidence. The earlier working system does not establish that new code is live.

## Current alignment

| Handout area | Current evidence | Remaining action |
| --- | --- | --- |
| Cloud VM, broker, database and API | AWS EC2, Mosquitto, SQLite and Flask are listed choices. PyTorch cutover reported matching model hash/new ID 5942, followed by local API 5943. Later public MQTT TLS and both HTTPS checks passed with fresh ID 6060. | Post-upload sensor/TLS/PUBACK checks passed in supplied output. The Ubuntu subscriber then received five sensor messages and the local API returned newer record 6746 at October 27 01:32:41 UTC. The user subsequently confirmed Android is working; no new screenshot or exact ID was supplied. Complete the new labelled PyTorch movement test. |
| TensorFlow or PyTorch; all axes, temporal learning, lightweight design | A trained 450-parameter PyTorch GRU uses ordered five-reading windows and all six axes. Verified Kaggle provenance/preparation and matching deployed model SHA-256 are recorded; ten model/contract tests passed on Ubuntu. | Perform a labelled live test for this model. Earlier Random Forest results do not measure PyTorch accuracy. |
| Rigorous model testing | New PyTorch development OOF: 2219/2258 (98.27%). Untouched latest-two-day holdout: 1197/1300 (92.08%). Ten actual model/database/API tests passed. | Repeat a labelled live movement test after deployment. This dataset contains one participant; no population-accuracy claim is justified. |
| Sensor and Wi-Fi firmware | The demonstrated RT-Spark has ICM20608 and RW007 on I2C2/SPI2. Earlier physical logs show sensor identity, networking, TLS, accepted MQTT login and PUBACKs. The user chose to continue with this hardware without an instructor reply. | CubeMX 6.18.1 output is now supplied, reviewed and reconciled with all application blocks retained. Post-upload logs now show sensor identity/init, Wi-Fi/DHCP, verified TLS and PUBACKs 1–7. The local Ubuntu subscriber and database-backed API are now checked after upload (five messages and record 6746). Android is now user-confirmed working. Retain the reference-hardware difference and complete the new movement test. |
| AWS coreMQTT and default-config define | Actual unmodified coreMQTT 2.3.1 low-level API now replaces the custom packet encoder/parser. `MQTT_DO_NOT_USE_CUSTOM_CONFIG` is defined. ARM build, five native suites and source guard passed. After CubeMX reconciliation, the user's Windows rebuild at `b44d2ba` passed the guard and succeeded in 24.05 s, SRAM 66,756 bytes and flash 162,780 bytes. | CubeMX review, local build and ST-LINK programming/verification are complete in supplied output (Verified OK; SUCCESS in 12.62 s). Post-upload sensor/network/MQTT runtime now includes seven PUBACKs. Post-upload Ubuntu broker/API readback now shows five messages and newer prediction 6746. Android operation is now user-confirmed; perform the labelled movement trial. The earlier custom-encoder PUBACKs are historical. |
| One reading/message per second | Firmware schedules a nominal one-second publish interval. Post-upload logs now contain PUBACK IDs/counts 1–7; earlier custom-encoder logs contain 1482–1498. | Obtain a timestamped capture to measure cadence. An untimed log cannot establish it. |
| Android smartphone, live predictions and history | Kotlin, Android Studio and Volley match listed choices. Earlier physical Xiaomi target and exact API match are preserved. Post-PyTorch screens show LIVE record 6067 and newest-first History 6068–6064, all at displayed 99.9% confidence. | Earlier supplied display checks are preserved; after the new upload/cloud check, the user confirmed Android is working without new screenshots or exact IDs. Proceed to movement validation; no new measured refresh timing is claimed. |

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

Source changes are prepared and tested, and the user-supplied Ubuntu output
now establishes the PyTorch cutover and new local API record. Subsequent public
checks passed and Android Live/History screens show later predictions. Actual
CubeMX 6.18.1 output is reviewed, the Windows rebuild passed, and the subsequent
ST-LINK upload reports programming complete and Verified OK. The subsequent
serial excerpt shows sensor initialization, a DHCP lease, verified TLS and
PUBACKs 1–7. A subsequent Ubuntu subscriber received five messages and the
local API returned newer database-backed record 6746 at October 27 01:32:41 UTC.
Post-upload Android operation is now user-confirmed, without new images or
exact displayed IDs. The first PyTorch movement attempt stopped after invalid prediction coverage
and a stale-record preflight; no accuracy is available. The later complete
session transcription confirms only a pre-start row. Later broker receipt and
five-second predictions are working again. The trial-time journal now
confirms six delivery-gap resets, including a 44-second receipt pause and
79 seconds between predictions. Check continuity under the intended power
setup before a new valid protocol; the underlying interruption remains unproven. The excerpt omits the version banner
and does not measure cadence. Strict completion remains pending those
applicable downstream and movement actions. The hardware/toolchain differences
remain explicitly recorded under the user's actual-board continuation choice.

The integration checklist includes additional engineering checks such as DHCP
renewal, controlled interruptions and memory/polling measurements. Their
pending status is recorded honestly; the handout does not prescribe every one
as a separate grading requirement, nor a particular video/report format.

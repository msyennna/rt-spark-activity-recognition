# Test and demonstration evidence

This index highlights the records needed to review the finished project.
Dated files retain the observations and limits of their original checkpoint.
For the current project status, read [LAB_COMPLETION.md](../LAB_COMPLETION.md).
For runnable offline checks, see [tests/README.md](../../tests/README.md).
The [October 28 repository review](2026-10-28_repository_review.md) records the
checks run while preparing the complete project for `main`.

## Final PyTorch result

The completed six-trial live session contains **29/53 correct windows (54.7%)**:
Walking **26/26**, jogging/Running **3/27**. All six trials passed the existing
quality rules, and independent replay matched the uploaded original session.

| Record | Contents |
| --- | --- |
| [Original-session audit](2026-10-27_pytorch_walk_jog_audit.md) | Final verified trial totals, confusion matrix and evidence limits |
| [Machine-readable audit](2026-10-27_pytorch_walk_jog_audit.json) | Anonymized independent audit output |
| [Reported session summary](2026-10-27_pytorch_walk_jog_reported.md) | Participant-supplied session summary and follow-up audit status |
| [Training report](pytorch_training_2026-10-26.json) | Dataset preparation, day-grouped development evaluation and chronological holdout |
| [Evaluation procedure](TIMED_VALIDATION.md) | Independent labels, timing, coverage and scoring rules |

Dataset development/holdout scores and live board accuracy describe separate
evaluations. The older **51/52 (98.1%)** result belongs to the Random Forest
baseline and is preserved in the
[October 25 report](2026-10-25_cubemx_walk_jog_trial.md) and
[original session folder](timed_walk_jog_20261025T153506Z_0abffb/).

## Firmware and actual board

| Record | Contents |
| --- | --- |
| [CubeMX review](2026-10-27_cubemx_generation_review.md) | Supplied generated output, preserved application blocks and source reconciliation |
| [Reconciled Windows build](2026-10-27_windows_reconciled_build.md) | Successful local build after the CubeMX review |
| [ST-LINK programming](2026-10-27_coremqtt_stlink_upload.md) | Programming and OpenOCD verification |
| [Device runtime](2026-10-27_coremqtt_runtime.md) | Sensor initialization, DHCP, TLS and MQTT acknowledgements |
| [Diagnostic firmware upload](2026-10-27_mqtt_diagnostic_upload.md) | Later diagnostic image's verified programming |
| [Extended publishing](2026-10-27_mqtt_diagnostic_continuation.md) | Reported excerpt containing 620 consecutive PUBACKs |
| [Power-bank walking capture](2026-10-27_power_bank_walk_receipts.md) | Latest requested 70-message receipt check and observed timing |

## Cloud and Android

| Record | Contents |
| --- | --- |
| [PyTorch deployment](2026-10-26_pytorch_cloud_deployment.md) | Model startup hash, Ubuntu checks and fresh API predictions |
| [Public checks and Android screens](2026-10-26_pytorch_android_public.md) | TLS/HTTPS observations and the saved Live/History screenshots |
| [Post-upload cloud check](2026-10-27_coremqtt_cloud_check.md) | Broker messages and a current database-backed API record |
| [Physical-phone comparison](2026-10-26_android_history.md#physical-phone-verification) | Earlier phone Live/History observations and API comparison |

The remaining dated files preserve earlier attempts and diagnostics, including
the stopped PyTorch trial and continuity investigations. They remain separate
from the final valid session; acknowledgement counts and UI confidence values
do not establish classification accuracy.

# Laboratory Activity 3: completion review

Reviewed October 26, 2026 (Asia/Manila), against the user-supplied
`BCA180 ESP - Laboratory Activity 3 (2).docx` and integration source/evidence.
The handout describes cloud/model, device firmware and mobile-app tasks.
This review records functional evidence and remaining alignment issues; it
does not certify a grade or replace the instructor's requirements.

## Demonstrated functional workflow

| Area | Evidence | Status and limit |
| --- | --- | --- |
| Sensor readings and direct board networking | Physical-board ICM20608 startup/readings, Wi-Fi/DHCP, TLS and MQTT login observations; October 26 transcript with 17 accepted PUBACKs, IDs 1482–1498 | Board-to-broker operation observed. The untimed transcript does not measure the requested one-second cadence. |
| Cloud broker, classifier, database and API | Earlier Mosquitto subscriber messages, audited stored prediction rows, and supplied current API responses | Working deployed pipeline observed. PUBACK and database completion remain distinct stages. |
| Machine-learning inference and activity test | Saved model and training source; six independently timed walking/jogging trials with preserved raw data | Audited 51/52 agreement (98.1%) on this participant/placement. This is not population accuracy or reproduced training cross-validation. |
| Android live display and history | Emulator record matches, physical Xiaomi target and History photo, physical-phone LIVE record 5167 matching the API | Main phone display functions observed. The phone timestamp correctly corresponds to UTC+08:00. Precise polling and a new phone outage/recovery experiment were not measured. |
| Build/source verification | Previously recorded firmware builds, five native C suites, five actual-library TLS cases and CubeMX source guard | Previously passed checks remain recorded. Documentation-only milestones have not changed application code. |

Evidence:

- [PUBACK report and transcript](testing/2026-10-26_cubemx_puback.md)
- [Physical-phone History and Live/API report](testing/2026-10-26_android_phone.md)
- [Audited movement trial](testing/2026-10-25_cubemx_walk_jog_trial.md)
- [Integration verification record](STM32_INTEGRATION.md)

## Handout alignment to resolve

### Machine-learning framework

The handout's software list names **TensorFlow, PyTorch** (page 2).
The checked-in `cloud/classifier/train_model.py` imports scikit-learn and trains
a 200-tree `RandomForestClassifier`. It is a working machine-learning classifier,
but it is not a TensorFlow or PyTorch implementation.

Acceptance of an alternative framework has not been established in the
supplied evidence. Resolve whether those named frameworks are required or
alternatives are accepted before declaring every handout item complete. If a
named framework is required, plan and validate the model change with the
correct training dataset, retaining the current working model and results.
This review does not silently replace the deployed classifier.

### CubeMX workflow

The repository contains the reviewed `.ioc`, STM32Cube/HAL sources, a passing
PlatformIO build and source guard. Actual CubeMX regeneration during this
integration has not been exercised. The active PlatformIO sources are
`firmware/src` and `firmware/include`; generated `Core` files are separate.

To establish that workflow, follow the scratch-project procedure in
[firmware/README.md](../firmware/README.md): seed the active USER CODE,
regenerate with the installed CubeMX version, run the generated-code guard,
review peripheral ownership/settings, then build reviewed changes. Record
what actually ran. Do not count the existing source guard as a regeneration run.

The handout's LSM6DSL/ISM43362/SPI3 examples differ from the physical
ICM20608/RW007/SPI2 hardware. The hardware-specific substitutions are documented.
The MQTT implementation is the reviewed foreground publisher rather than the
handout's coreMQTT example. Keep these implementation choices explicit when
presenting the project.

## Model reproducibility and interpretation

The training script expects `dataset/dataset.csv`; the user's earlier search
reported it missing, and recovery has not been documented. It is intentionally
excluded from Git, so repository absence alone is not a new local-file check.
Recover the correct data privately, document its provenance, label meanings,
units and sampling rate, and retain training/evaluation output if reproducing
the saved model. Do not invent cross-validation results from source code.

The model uses all six sensor axes and magnitude statistics over five-reading
windows. Those aggregate features summarize time windows but are invariant to
sample order; explicit temporal-correlation learning is not established. The
handout encourages temporal information and robust generalization. Explain
this limitation and the single-participant trial scope accurately.

The 67.5% values shown in the later app captures are prediction
confidence, not measured accuracy. The actual activity in those captures was
not labelled, and the current model has WALKING/RUNNING classes only.

## Scope of remaining engineering checks

The integration checklist also records cadence measurement, DHCP renewal,
controlled network interruption, board negative-certificate cases and memory/
polling measurements. These are useful engineering checks; the handout does
not individually prescribe every one as a separate grading test. Their
pending status should not obscure the demonstrated core workflow, and they
must not be reported as passed without evidence.

No app reinstall, firmware reflash, cloud reprovisioning, retraining, merge to
main or new movement trial was performed for this documentation milestone.

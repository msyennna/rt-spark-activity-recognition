# Handout alignment and deployment status

The authoritative reference is the supplied
BCA180 ESP - Laboratory Activity 3 (3).docx, SHA-256
8efc5cf8d9c75abff183d1f76581beaded74cc0c1460bce08006ef6a08ef6bc8.
All seven pages, including the configuration screenshots, were reviewed.
The user explicitly requests the listed suggestions, without substituting
unlisted software choices.

## Already matching choices

AWS EC2, Mosquitto, Flask, SQLite, Android Studio, Kotlin and Volley are all
listed choices or recommendations. The Android application has been shown on
a physical Xiaomi device with a matching server prediction. No replacement
of these components is needed to follow the handout.

## PyTorch migration

The repository now uses the listed PyTorch framework: a 450-parameter temporal
GRU trained on the handout's verified Kaggle version-2 dataset. See
[the model documentation](../cloud/classifier/README.md) for preparation,
evaluation, artifact integrity and test results.

The source and checkpoint have been prepared/tested here. **The Ubuntu service
has not yet been updated.** Its previous live evidence describes the deployed
Random Forest baseline until a model cutover is performed. New PyTorch
predictions must receive their own live validation and movement trial.

A deployment should preserve a backup of the current cloud/classifier and
cloud/mqtt files and stop only the classifier service for the cutover. Install
the CPU PyTorch dependency in the service's existing virtual environment,
copy the new model/metadata and classifier modules together, run the offline
inference/contract checks, then restart that classifier and confirm new rows
and phone output. The service's actual systemd unit name must be inspected;
it is not assumed by these instructions. Do not replace Mosquitto, Nginx,
certificates, credentials, or the existing activity database for this change.

## Firmware requirements and a handout conflict

The custom MQTT encoder has been replaced by the actual AWS coreMQTT 2.3.1
serializer/deserializer, with the handout's MQTT_DO_NOT_USE_CUSTOM_CONFIG
definition applied to the library. Five native suites, an ARM firmware build,
and the source guard passed. See [the exact build/test record](testing/2026-10-26_coremqtt_port.md).
This firmware has not yet been flashed or observed on the physical board;
earlier PUBACKs describe the previous custom-encoder implementation.

The handout specifies RT-Spark on page 1 and VSCode PlatformIO on page 2,
but its later reference steps name
LSM6DSL, ISM43362, SPI3, CMSIS driver components, and an AC6 solution.yml.
The demonstrated board/project uses ICM20608, RW007, SPI2 and STM32Cube/HAL
with PlatformIO. These are concrete hardware/toolchain differences.

A driver name or SPI bus cannot be changed as a cosmetic edit: software for an
absent chip will not operate the installed one. Strict literal conformance for
those reference steps requires confirmation/correction of the intended board
and driver/toolchain instructions. This conflict remains open; it is not
silently waived or labelled complete. Do not configure SPI3 or select an
ISM43362/LSM6DSL driver merely to match a screenshot on the existing wiring.

The handout's cpackget/component-editor/AC6 steps also do not directly describe
the existing PlatformIO project. Using the real coreMQTT library can close the
MQTT-library difference, but does not by itself complete those reference
toolchain steps or supply the missing WiFi_ISM43362_HW.c reference file.

Actual CubeMX regeneration still needs to run in the user's installed CubeMX,
following the scratch-copy and review procedure in
[firmware/README.md](../firmware/README.md). A source-guard pass alone is not a
generator run.

## Completion claim

The existing end-to-end baseline works. Handout alignment is in progress;
the project must not be presented as strictly complete while the new model
is undeployed or the hardware/toolchain conflict remains unresolved.
Historical evidence is retained with its original implementation attribution.

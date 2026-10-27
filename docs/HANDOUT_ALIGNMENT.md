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

**The Ubuntu classifier has now been updated.** User-supplied terminal output
reports a matching model startup hash and a new prediction ID 5942 at
2026-10-26 04:08:05 UTC; the subsequent local API response returned ID 5943.
Ten model/contract tests also passed on Ubuntu before cutover. See
[the deployment evidence](testing/2026-10-26_pytorch_cloud_deployment.md).
Subsequent public TLS/HTTPS checks all passed with record 6060; supplied
Android screens show LIVE record 6067 and History through 6068 at displayed
99.9% confidence. See [the post-cutover public/Android evidence](testing/2026-10-26_pytorch_android_public.md).
The screens were captured later than the checker; an exact server-row match
for those screen IDs is not claimed. A labelled PyTorch movement trial remains
pending. Earlier Random Forest observations retain their original attribution.

Use [the Windows/Ubuntu deployment runbook](DEPLOY_HANDOUT_UPDATES.md).
Its explicit-file bundle passed all ten actual model/database/API staging
tests. Four helper tests cover service scope, restore after failure or
interruption, and the fresh-ID/matching-model-hash success gate. These local
tests do not claim a live server cutover.

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
The local reconciled image has now been programmed through ST-LINK, with
OpenOCD Verified OK and SUCCESS in 12.62 seconds in the user's supplied output.
See [the upload record](testing/2026-10-27_coremqtt_stlink_upload.md). The
subsequent [runtime excerpt](testing/2026-10-27_coremqtt_runtime.md) shows sensor
initialization, DHCP BOUND, verified TLS and seven PUBACKs. The version banner
is outside the excerpt; attribution follows the build/upload sequence. Earlier
PUBACKs retain their previous custom-encoder attribution.

The subsequent Windows build also passed in 16.75 seconds, with coreMQTT 2.3.1,
66,756 bytes static SRAM and 162,788 bytes flash. The private-header existence
check was True; its values were not inspected. See
[the preserved build evidence](testing/2026-10-26_windows_coremqtt_build.md).
The subsequent user-supplied CubeMX 6.18.1 output now provides generation
evidence. Its reviewed peripheral changes are reconciled into the active
sources; the generated I2C2 main/MSP duplicates were removed. All 17 nonempty
application USER CODE blocks survived. See
[the generation review](testing/2026-10-27_cubemx_generation_review.md).

The handout specifies RT-Spark on page 1 and VSCode PlatformIO on page 2,
but its later reference steps name
LSM6DSL, ISM43362, SPI3, CMSIS driver components, and an AC6 solution.yml.
The demonstrated board/project uses ICM20608, RW007, SPI2 and STM32Cube/HAL
with PlatformIO. These are concrete hardware/toolchain differences.

On October 26 the user confirmed that no instructor clarification had arrived,
but chose to continue with the installed RT-Spark hardware. The implementation
therefore proceeds with ICM20608/I2C2, RW007/SPI2 and the existing STM32Cube/HAL
PlatformIO project. This is the user's continuation decision, not instructor
approval or evidence that the different reference hardware is installed.

A driver name or SPI bus cannot be changed as a cosmetic edit: software for an
absent chip will not operate the installed one. Strict literal conformance for
those reference steps requires confirmation/correction of the intended board
and driver/toolchain instructions. That literal difference remains documented;
it no longer blocks proceeding with the user's actual-board implementation.
Do not configure SPI3 or select an
ISM43362/LSM6DSL driver merely to match a screenshot on the existing wiring.

The handout's cpackget/component-editor/AC6 steps also do not directly describe
the existing PlatformIO project. Using the real coreMQTT library can close the
MQTT-library difference, but does not by itself complete those reference
toolchain steps or supply the missing WiFi_ISM43362_HW.c reference file.

Actual CubeMX 6.18.1 output was supplied and reviewed. The original ZIP,
seed hashes, accepted source hashes and checks are preserved in the generation
review. Future regeneration follows the scratch-copy/review procedure in
[firmware/README.md](../firmware/README.md); a guard pass alone is not a
generator run or a physical test.

## Completion claim

The existing end-to-end baseline works. Handout alignment is in progress;
new-model movement checks and coreMQTT physical verification remain pending.
CubeMX generation output is now reviewed and reconciled. The user has chosen to continue with the actual
hardware; completion documentation must retain the hardware/toolchain
differences rather than claim that every conflicting reference step was run.
Historical evidence is retained with its original implementation attribution.

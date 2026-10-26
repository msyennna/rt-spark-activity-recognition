# RT-Spark activity recognition: STM32CubeMX integration setup

The active classifier source is now the handout's **PyTorch** framework, with
a 450-parameter temporal model and a verified training checkpoint. The Ubuntu
cutover reported PASS on October 26 at 04:08:05 UTC with a matching model hash
and new ID 5942, followed by local API ID 5943. See
[the deployment record](testing/2026-10-26_pytorch_cloud_deployment.md),
[HANDOUT_ALIGNMENT.md](HANDOUT_ALIGNMENT.md) and
[the model guide](../cloud/classifier/README.md). Earlier
Random Forest live results below retain their original attribution.

Post-cutover public checks now pass verified MQTT TLS and HTTPS latest/history
with record 6060. New Android screens show LIVE 6067 and History through 6068
at displayed 99.9% confidence. See
[the post-cutover report and screenshots](testing/2026-10-26_pytorch_android_public.md).
These display observations do not measure accuracy; the labelled PyTorch
movement trial and new coreMQTT physical verification are still pending.

The `integration/stm32cubemx` branch combines Shanice's STM32Cube/HAL firmware
with the existing cloud, trained model, provisioning scripts, and Android app.
The firmware uses PlatformIO and foreground lwIP networking. The unchanged
import and the direct MQTT/TLS port both build. Native source and cryptographic
tests pass. On October 23, 2026, user-provided physical-board logs showed sensor
readings, Wi-Fi/DHCP, verified TLS and accepted MQTT login. The Ubuntu subscriber
received five sensor messages, and the local API returned current record 3136.
The user subsequently reported a public service-checker pass and confirmed
Android live/history and power-cycle recovery in the emulator workflow.
See [STM32_INTEGRATION.md](STM32_INTEGRATION.md) for the evidence and remaining
measured cadence, network-interruption and regeneration checks. The [timed movement helper](testing/TIMED_VALIDATION.md) passes its
offline tests. The October 25 six-trial result is now checked against the
supplied raw files: 51/52 correct windows (98.1%). Independent time/ID selection,
quality checks and exact helper replay agree; both originals are preserved
unchanged. See the
[trial report](testing/2026-10-25_cubemx_walk_jog_trial.md).

On October 25, the public checker passed TLS/HTTPS checks but returned a stale
October 23 prediction, so the movement-test preflight stopped. A later board
reset excerpt shows Wi-Fi connected, DHCP started and sensor initialization;
a current DHCP lease, MQTT acknowledgement and fresh database output were still
unverified at that checkpoint. See the
[October 25 checkpoint](testing/2026-10-25_wifi_recovery.md).
The subsequent timed trial provides 82 distinct current prediction rows across
six valid intervals. Its 52 eligible windows and metrics are independently
reproduced; eligible prediction completion gaps are 5–6 seconds. Individual
sensor acquisition timestamps remain unmeasured. An October 26 serial excerpt
now records 17 accepted PUBACK messages, IDs 1482–1498; see the
[PUBACK report](testing/2026-10-26_cubemx_puback.md). Later Android History and Live cards match supplied local API records
4754 and 4877. Two Live images show LIVE with advancing prediction IDs,
supporting the requested automatic-refresh check. The user confirms the target
is the Android Studio emulator. A later photo establishes physical-phone
History display, with Xiaomi 25028RN03A selected in Android Studio. A subsequent
phone Live image shows LIVE and record 5167, matching local API WALKING, 67.5%,
and 02:08:39 UTC = 10:08:39 AM Asia/Manila. The physical-phone display/record
comparison is complete.

The target path is ICM20608 on an STM32F407 RT-Spark board, direct RW007 Wi-Fi,
MQTT over verified TLS, an Ubuntu classifier and SQLite database, an HTTPS API,
and the Android app. The earlier serial-to-MQTT gateway remains available in
`data_collection`; it is not required when the direct publisher is running.

## What is included

- `firmware/src` and `firmware/include`: compiled STM32Cube/HAL application,
  ICM20608 driver, RW007 transport, and foreground lwIP network interface.
- `firmware/platformio.ini`: active `black_f407zg` build configuration.
- `firmware/activity_recognition/activity_recognition.ioc`: CubeMX configuration;
  review its differences from the compiled files before regeneration.
- `cloud`: classifier, database, and HTTP API source already in the project.
- `android_app`: the working Gradle project, launcher `RtsparkActivity`, live view,
  prediction history, automatic refreshing, and stale-data detection.
- `tools/provisioning`: the MQTT TLS and Nginx HTTPS setup scripts used for this lab.
- `tools/evaluation`: independently timed walking/jogging evaluation and offline
  tests using temporary data; the evaluator reads the deployed database read-only.
- `tools/legacy/rtthread`: archived RT-Thread patch and flashing helpers. These
  target the previous firmware and are outside the active workflow.

The scripts and trust configuration target this lab's hostname and CA. They are
not an unattended installer for an unrelated server. Review their hostname,
fingerprint, repository path, prerequisites, and account settings before reuse.
The CA certificate is public. Its private signing key is not included.

## Firmware build and local configuration

1. Use the separate integration worktree on `integration/stm32cubemx`. Its base
   is the reviewed `feature/sensor-data-collection` commit; the original checkout
   and its private configuration remain separate.
2. Install/use PlatformIO Core, then build from this worktree's repository root:

   ```powershell
   pio run -d firmware
   ```

   This command builds only. It does not upload firmware. The first, unchanged
   Shanice import passed with 27,560 bytes RAM and 56,212 bytes flash.
   The integrated build pins the STM32 platform/framework/toolchain and Mbed
   TLS 3.6.7. On a clean checkout, its build script downloads the complete official
   Mbed TLS release and verifies its SHA256 before extraction. The dependency
   cache lives in ignored `firmware/.deps`.
3. For local Wi-Fi setup, copy `firmware/include/rw007_wifi_config.example.h` to
   the ignored `firmware/include/rw007_wifi_config.h`, then supply the SSID and
   password locally. Use valid C strings. The imported transport accepts an SSID
   of 1–32 bytes and a password of 0–31 bytes, with an empty password only for an
   open access point. Its 32-byte password wire field includes a terminating NUL.
   Do not assume the old RT-Thread credential limits apply to this transport.
4. Copy `firmware/include/rtspark_private.h.example` to the ignored
   `firmware/include/rtspark_private.h` and set the existing MQTT password and a
   unique client ID locally. The broker username remains `yena_sensor`. MQTT
   stays disabled while either private value is empty. Never commit credentials
   or built board images; completed images contain the configured secrets.
5. Review [CubeMX regeneration and hardware differences](STM32_INTEGRATION.md#cubemx-regeneration-and-hardware-differences)
   before regenerating. PlatformIO compiles `src`/`include`, not the ignored
   CubeMX `Core` output. The stale LSM6DSL selection was removed; the actual
   compiled driver remains ICM20608. The network interface now uses the station
   MAC returned by the actual RW007 module, with no shared fallback MAC.

The publishing contract to preserve is one reading per second to
`activity/sensor`, QoS 1, retain false, with exactly `acceleration_x`,
`acceleration_y`, `acceleration_z`, `gyro_x`, `gyro_y`, and `gyro_z`.
Acceleration is in g and angular velocity is in rad/s. The broker hostname is
`ec2-15-134-214-242.ap-southeast-2.compute.amazonaws.com`, TCP port 8883.
TLS must verify the existing lab CA, hostname, and certificate dates using UTC.
The matching public CA remains in Android's `app/src/main/res/raw/rtspark_ca.crt`.
The firmware copy is `firmware/include/rtspark_ca.h`.

The platform obtains UTC asynchronously from `time.cloudflare.com` using UDP
123, refreshes it hourly and refuses new TLS connections when UTC is unavailable
or over 24 hours old. This port has no RT-Thread shell time-setting command; a
network that blocks NTP will leave publishing waiting for UTC. Hardware RNG
requires the corrected 48 MHz PLLQ domain and fails closed on status faults,
timeout or a repeated word. Credentials are not logged by the MQTT module.

Each PUBACK confirms broker acknowledgement. It does not by itself confirm
database or Android processing. Keep `RW007_Network_Poll()` running while
waiting for DNS, TCP, UTC synchronization, TLS, and MQTT traffic. A blocking wait
that stops foreground polling also stops incoming packets and lwIP timers.

Run `python -B tests/firmware_host/run.py` for the native C tests and
`python firmware/scripts/check_cubemx.py` for the read-only configuration guard.
The host tests exercise actual MQTT/platform source with mocked peripherals and
TLS calls. They do not establish an actual board connection or TLS handshake.
The separate `python -B tests/firmware_tls/test_tls.py` suite uses the real Mbed
TLS client and temporary localhost certificates. Its five cases pass: a verified
ECDHE_RSA handshake and rejection of a wrong CA, wrong hostname, premature date
and expired date. See its [README](../tests/firmware_tls/README.md) for the
hash-checked restartable ECDHE_RSA adaptation and retest instructions.

The linker reserves a 16 KiB foreground stack. TLS uses a fixed 56 KiB arena in
the separate 64 KiB CCM bank; the C library uses an independently bounded 8 KiB
heap in SRAM, and lwIP uses its own bounded SRAM heap. TLS allocation failures
enter retry; C heap exhaustion returns `ENOMEM` with its bounds preserved.
Final bank usage is recorded separately in the integration milestone table.
The earlier custom-encoder ARM build used SRAM 66,756 bytes, separate CCM
57,344 bytes and flash 160,860 bytes. The later coreMQTT Windows build passed
with static SRAM 66,756 bytes and flash 162,788 bytes; see
[the supplied build log](testing/2026-10-26_windows_coremqtt_build.md).
That Windows log does not separately report CCM. The toolchain emits warnings about unused
libnosys system calls and the default RWX load segment; there are no build errors.

Initial sensor output, Wi-Fi/DHCP, board TLS/login, broker reception and one local
API record have been observed. Later public connectivity and Android
live/history/recovery checks were confirmed by the user; see the
[live report](testing/2026-10-23_cubemx_live_pipeline.md) for their scope.
Extended physical checks remain pending. Do not use the archived RT-Thread flashing helper for the
PlatformIO image. Flashing is a separate, explicit manual operation.

## Ubuntu services

The working repository is `/home/ubuntu/rt-spark-activity-recognition`. Python
dependencies and the trusted PyTorch checkpoint/metadata pair must be present in the
environment used by the classifier and API. The loader verifies the model SHA-256 and uses
weights-only state loading; deploy the checkpoint and metadata together.

The deployed services are:

| Component | Listener | Purpose |
| --- | --- | --- |
| Mosquitto local MQTT | `127.0.0.1:1883`, `[::1]:1883` | Local classifier access |
| Mosquitto MQTT TLS | TCP 8883 | Authenticated board publisher |
| Gunicorn | `127.0.0.1:5000` | Flask API |
| Nginx HTTPS | TCP 443 | API access for the app |

`tools/provisioning/configure_rtspark_mqtt_tls.py` expects the existing lab CA and
server certificate/key in `/home/ubuntu/rtspark-mqtt-pki`; it verifies that setup,
prompts locally for the broker password, tests an isolated broker, and installs
the checked Mosquitto configuration. It does not create the CA. The HTTPS script
expects the existing certificates and running Gunicorn API. Both scripts are
already applied on the current server; do not rerun them merely to commit code.

Private signing keys, broker password files, deployed certificates' private keys,
server service state, and the live SQLite database are not copied into GitHub.
The provisioning scripts preserve the configuration procedure in source form.
The existing classifier/Gunicorn systemd unit files are not included in this
export; record those separately if recreating the VM from scratch.

Allow SSH 22, MQTT TLS 8883, and HTTPS 443 from the intended client public IPs in
the EC2 security group. The classifier's MQTT 1883 and Gunicorn 5000 remain local.

## Android

Open `android_app` in Android Studio, sync Gradle, then run the `app` configuration
on an emulator or Android device. The current project uses AGP 9.4.1 and SDK 37;
the Gradle wrapper and version catalog are committed. Android Studio's generated
Compose template remains alongside the native-view `RtsparkActivity` launcher.

The app trusts `app/src/main/res/raw/rtspark_ca.crt` for the configured broker
hostname only, uses HTTPS, and requests latest/history through Volley. It contains
no Wi-Fi or MQTT password. `local.properties` is specific to the developer's PC
and is excluded. A physical phone needs network access allowed by the EC2 rules.

Android History images reviewed October 26 show visible IDs 4585–4580 and,
later, 4754–4749 in newest-first order. Follow-up record 4754 matches the supplied
local API: WALKING, confidence 0.675 (67.5%), and timestamp 2026-10-25 16:32:23
UTC with matching displayed clock fields. The API contains nine newer rows at
five-second completion intervals. The counter reports 50 Walking and 0 Running
among the latest 50; the earlier movement-test records are older. See the
[history/API observation](testing/2026-10-26_android_history.md) and retained API
response.

A later [Live/API check](testing/2026-10-26_android_live.md) shows LIVE in two
images, prediction IDs advancing from 4874 to 4877, and recorded timestamps
15 seconds apart. Live record 4877 matches the supplied local API: WALKING,
67.5%, timestamp 2026-10-25 16:42:41 UTC with matching displayed clock fields.
The sequence supports the requested automatic-refresh check. The still images
do not independently measure polling precision or intervening user input.
The local curl checks do not independently repeat public HTTPS trust verification.

The user explicitly confirms that the October 26 Live-check workflow runs on
the virtual phone inside Android Studio. Together with the matching local API
records, this establishes the emulator display workflow. A subsequent
[physical-phone photo and device-selector image](testing/2026-10-26_android_phone.md)
show populated History on physical hardware and Xiaomi 25028RN03A selected in
Android Studio. Visible phone records are 5027–5023, Walking at 67.5%, newest
first. A subsequent phone Live image shows LIVE and record 5167, matching
local API WALKING, 67.5%, and 2026-10-26 02:08:39 UTC = 10:08:39 AM Asia/Manila.
The supplied limit=20 API response contains IDs 5169–5150 with 5–6-second
completion gaps. This completes the physical-phone display/record comparison.

### Run the same app on a physical Android phone

Keep the RT-Spark board powered and publishing. Connect the phone to the same
Wi-Fi network as the laptop so it can use the already allowed network path to
the lab API.

1. On the phone, find **Build number** in Settings > About phone (sometimes
   under Software information), then tap it seven times to enable Developer
   options. Open Developer options and enable **USB debugging**.
2. Connect the unlocked phone to the laptop with a USB data cable. Accept the
   phone's **Allow USB debugging?** prompt for this computer.
3. In Android Studio, use the existing working `android_app` project. Select the
   phone's model in the target-device dropdown, select the `app` configuration,
   and click **Run**.
4. In the installed app on the phone, check **Live activity** for LIVE status and
   an advancing prediction number; open **History** and check newest-first rows.
   Record the phone model and capture matching prediction IDs for the evidence.
5. If the phone is absent from the target list, use **Tools > Troubleshoot Device
   Connections**. Windows may need the phone manufacturer's ADB USB driver.

The user has now supplied physical-phone History and LIVE evidence, including
a direct server-record match for Live prediction 5167. These installation
steps remain available for reproducing the setup. The single phone Live image
does not independently measure refresh precision or repeat a power-interruption
recovery; earlier emulator observations retain their original scope.
See the official Android instructions for
[developer options](https://developer.android.com/studio/debug/dev-options) and
[running on hardware](https://developer.android.com/studio/run/device).

For a future model evaluation, follow
[the timed walking/jogging protocol](testing/TIMED_VALIDATION.md); the helper
records activity intervals before comparing predictions and preserves errors.
The completed six-trial result remains preserved; installing the app on a phone
does not require repeating it.

## Historical baseline verification on the actual lab setup

The following results describe the RT-Thread baseline on October 20, 2026.
They do not verify the STM32CubeMX integration:

- The board's ARM firmware built and ran on the physical board.
- Direct Wi-Fi MQTT TLS readings arrived at the Ubuntu broker.
- The classifier saved predictions and both HTTPS endpoints returned records.
- The Android project built, launched in the emulator, displayed current
  predictions, and showed history newest first at approximately five-second intervals.
- Removing board power caused WAITING FOR SENSOR while preserving the last
  reading. Restoring power returned the app to LIVE automatically.
- Movement-trial screenshots showed 42 WALKING and 8 RUNNING records among the
  latest 50. The eight RUNNING confidences ranged from 71.0% to 99.0%.
  These observations are not a measured accuracy score or a confusion matrix.

## Model limitations

The active PyTorch GRU consumes five ordered one-second readings, all six axes
and two derived magnitudes. Timestamp-based preparation, rotation augmentation
and recording-day splits are documented in
[the model guide](../cloud/classifier/README.md). The handout's Kaggle dataset
has been recovered and verified; its CSV stays excluded from Git.

The new offline results are 98.27% development out-of-fold agreement and 92.08%
on the untouched latest-two-day holdout, from one participant. The deployment
checkpoint is refit afterward; neither result is a live board test. The prior
98.1% six-trial result belongs to the earlier Random Forest baseline.
The Ubuntu service now runs PyTorch, but its fresh movement accuracy has not
yet been measured. These scores must stay separate.

Only WALKING and RUNNING are trained; a resting board can receive either label.
Confidence is a per-prediction estimate. API timestamps are UTC, and Android
displays the device's timezone.

## Completion review

The board-to-broker acknowledgement capture and physical-phone Live/API match
are now recorded. See [LAB_COMPLETION.md](LAB_COMPLETION.md) for the handout
comparison. The handout names TensorFlow/PyTorch; the active source now uses
PyTorch, and the Ubuntu cutover/local API check is now recorded. Public checks
and new-model Android Live/History are also observed. A labelled PyTorch
movement trial, coreMQTT physical verification and actual CubeMX
regeneration remain pending. Training-data provenance and reproducibility are
documented in the model guide; the handout hardware/toolchain conflict remains open.

## Git workflow

Commit and push each completed integration milestone to
`integration/stm32cubemx`. Keep `main` and `feature/sensor-data-collection` intact.
Record build results separately from hardware and live-service evidence in
[STM32_INTEGRATION.md](STM32_INTEGRATION.md); do not count pending physical
checks as passes.

Review staged paths and the staged diff locally before each commit. Root secret
exclusions remain in force, including private headers, environment files,
database files, signing keys, and compiled board images. Do not copy private
configuration or generated build directories between worktrees into tracked
paths. Keep the Gradle wrapper JAR: it is a required build tool.


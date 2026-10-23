# RT-Spark activity recognition: STM32CubeMX integration setup

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
PUBACK, measured cadence, physical-phone, network-interruption and regeneration
checks. A repeatable [timed movement test](testing/TIMED_VALIDATION.md) is ready;
its offline tests pass, but integration movement results are not yet recorded.

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
The integrated ARM build passes with SRAM 66,756 bytes, separate CCM 57,344
bytes and flash 160,860 bytes. The toolchain emits warnings about unused
libnosys system calls and the default RWX load segment; there are no build errors.

Initial sensor output, Wi-Fi/DHCP, board TLS/login, broker reception and one local
API record have been observed. Later public connectivity and Android
live/history/recovery checks were confirmed by the user; see the
[live report](testing/2026-10-23_cubemx_live_pipeline.md) for their scope.
Extended physical checks remain pending. Do not use the archived RT-Thread flashing helper for the
PlatformIO image. Flashing is a separate, explicit manual operation.

## Ubuntu services

The working repository is `/home/ubuntu/rt-spark-activity-recognition`. Python
dependencies and the trusted `activity_model.joblib` must be present in the
environment used by the classifier and API. Do not load model files from unknown
sources: joblib deserialization can execute code.

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

The October 23 user confirmation covers the emulator workflow. A physical-phone
run has not yet been established. For the next model evaluation, follow
[the timed walking/jogging protocol](testing/TIMED_VALIDATION.md); the helper
records activity intervals before comparing predictions and preserves errors.

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

The saved model recognizes only WALKING and RUNNING. A resting board can receive
either label because REST is not a trained class. The displayed confidence is
the model's probability estimate, not a validated percentage accuracy.

The classifier consumes five one-second readings per non-overlapping window.
Training uses the same feature extraction and a downsampling factor of five for
the original approximately 5 Hz data. The original `dataset/dataset.csv` is absent
from the current project, so retraining requires recovering the correct dataset,
its label meanings, units, sampling rate, and provenance. The existing trained
artifact can be used for inference without the CSV.

The API stores SQLite timestamps in UTC; Android displays the device's timezone.
An emulator set to UTC will show a time eight hours behind Manila.

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


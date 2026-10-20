# RT-Spark activity recognition: setup and verified behavior

The working path is ICM20608 on an STM32F407 RT-Spark board, direct RW007 Wi-Fi,
MQTT over verified TLS, an Ubuntu classifier and SQLite database, an HTTPS API,
and the Android app. A Windows serial-to-MQTT gateway is an earlier optional path;
the current board publishes directly and does not need that gateway running.

## What is included

- `firmware/activity_recognition/applications`: direct board publisher and public CA.
- `cloud`: classifier, database, and HTTP API source already in the project.
- `android_app`: the working Gradle project, launcher `RtsparkActivity`, live view,
  prediction history, automatic refreshing, and stale-data detection.
- `tools/firmware`: reviewed mbedTLS port fixes that must survive a package download.
- `tools/provisioning`: the MQTT TLS and Nginx HTTPS setup scripts used for this lab.
- `tools/flashing/flash_rtspark_wifi.py`: checked, block-based flashing helper.

The scripts and trust configuration target this lab's hostname and CA. They are
not an unattended installer for an unrelated server. Review their hostname,
fingerprint, repository path, prerequisites, and account settings before reuse.
The CA certificate is public. Its private signing key is not included.

## Board setup

1. Open the existing `firmware/activity_recognition` project in RT-Thread Studio.
   It uses RT-Thread 4.1.1, RW007 v2.0.1, ICM20608, Paho MQTTPacket, and
   mbedTLS v2.7.10.1. Download the packages selected in the tracked `.config`.
2. From the repository root in Windows PowerShell, run:

   ```powershell
   python .\tools\firmware\apply_mbedtls_fixes.py
   ```

   Reapply the fixes after re-downloading mbedTLS. They enable certificate date
   validation and the time callback, add POSIX declarations needed by the port,
   and replace its pseudo-random entropy stub with the STM32 hardware RNG adapter.
3. Copy `applications/rtspark_private.h.example` to `applications/rtspark_private.h`
   within the firmware project. Enter your local Wi-Fi and MQTT credentials in
   the copied file. Use valid C strings, escaping quotes and backslashes if needed.
   The tested configuration uses Wi-Fi passwords of 8–32 ASCII characters and an
   MQTT password of 12–64 characters, matching the broker account `yena_sensor`.
   Use a unique MQTT client ID for each board. Keep the completed header private.
4. The CA in `rtspark_ca.h`, the hostname in the application, and the broker's
   server certificate must agree. A different CA/server requires updating both
   the board and Android trust configuration; keep certificate verification on.
5. Refresh, clean, and build the project in RT-Thread Studio. Keep all built
   firmware private because credentials are embedded in it.
6. The supplied flashing helper is pinned to the original lab's ST-LINK serial
   number and checks both ELF and BIN files. For that board, run the helper from
   PowerShell after closing active debug sessions. For another board, review the
   target checks and programmer path first. Do not bypass a failed identity check.

The working board obtains an IP address, synchronizes UTC, verifies TLS 1.2, and
publishes one six-field sensor reading per second to `activity/sensor` with QoS 1.
Acceleration is in g and angular velocity is in rad/s. Each PUBACK confirms broker
acknowledgement. It does not by itself confirm database or Android processing.

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

## Verified on the actual lab setup

On October 20, 2026:

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

The preparation helper updates ignore rules, exports only public support files,
checks staged content against the current private board passwords and private-key
markers, and stages the reviewed project paths. It does not commit or push.
This targeted check does not claim to find every possible unrelated credential.

Review `git diff --cached --stat` and, locally, `git diff --cached` before a commit.
Never post a diff containing passwords. Commit to `feature/sensor-data-collection`
and push normally to origin. Keep the Gradle wrapper JAR: it is a required build
tool, not a compiled board binary containing credentials.
